/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/flash.h"
#include "hardware/flash.h"
#include "pico/bootrom.h"
#include "boot/picobin.h"
#include "ffs_internal.h"
#include "pico/ffs.h"

#define NUMBER_OF_CHUNKS                2

#define PT_ID_LOCATION_AND_FLAGS \
    (PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_PARTITION_ID)
#define PT_SINGLE_LOCATION_AND_FLAGS \
    (PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION)

// Used to mark flash offset addresses as 'not in use'
#define NOT_A_VALID_FLASH_OFFSET        0xffffffff

// ffs uses *offsets* from the flash start throughout. All flash accesses,
// including reads, go via the bootrom flash API which takes untranslated
// flash storage addresses - flash is never read through a memory pointer.
#define FL_OFFSET_TO_ADDR(_o)           (XIP_BASE + (uint32_t)(_o))

// Size of the stack buffers used when reading file data from flash
#define FLASH_READ_BUFFER_SIZE          FLASH_PAGE_SIZE

#ifndef PICO_RP2040
// Flag bit definitions for ROM flash API
#if PICO_NONSECURE
#define FSS_CFLASH_OP_SECURITY CFLASH_SECLEVEL_VALUE_NONSECURE
#else
#define FSS_CFLASH_OP_SECURITY CFLASH_SECLEVEL_VALUE_SECURE
#endif
static const cflash_flags_t cflash_read = {
    .flags = (CFLASH_OP_VALUE_READ << CFLASH_OP_LSB) |
             (FSS_CFLASH_OP_SECURITY << CFLASH_SECLEVEL_LSB) |
             (CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB)
};

static const cflash_flags_t cflash_program = {
    .flags = (CFLASH_OP_VALUE_PROGRAM << CFLASH_OP_LSB) |
             (FSS_CFLASH_OP_SECURITY << CFLASH_SECLEVEL_LSB) |
             (CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB)
};

static const cflash_flags_t cflash_erase = {
    .flags = (CFLASH_OP_VALUE_ERASE << CFLASH_OP_LSB) |
             (FSS_CFLASH_OP_SECURITY << CFLASH_SECLEVEL_LSB) |
             (CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB)
};
#endif

// Convert an unaligned flash offset into its page start offset
#define ALIGN_TO_START(_o, _a)          ((_o) & ~((_a)-1))
#define FL_OFFSET_TO_PAGE_START(_o)     ALIGN_TO_START((_o), FLASH_PAGE_SIZE)

typedef enum {
    FILE_FOUND_VALID   = 0
  , FILE_FOUND_NASCENT = 1 // means finalisation did not complete
  , FILE_FOUND_DEPRECATED = 2
  , VIRGIN_FLASH       = 3
  , UNEXPECTED_DATA    = 4
  , FLASH_READ_ERROR   = 5
} file_found_e;

typedef struct {
    uint32_t increment;
    uint32_t check_offset;
    uint32_t end_offset;
    file_found_e found_filter;
    file_found_e file_found;
    ffs_file_hdr_t file_header;
} flash_iterator_t;

typedef struct {
    uint8_t  cached_flash_page[FLASH_PAGE_SIZE];
    uint32_t wr_page_cache_offset;
    uint32_t start_offset;
    uint32_t end_offset;
    uint32_t chunk_size;
    uint32_t next_write;
    bool initialised;
} ffs_data_t;

static ffs_data_t ffs_data = { 0 };

#ifndef PICO_RP2040
// This code copied from: pico-sdk/src/rp2_common/pico_cyw43_driver/cyw43_driver.c
static int get_ffs_partition_info(void) {

    // maximum of 16 partitions, each with maximum of 4 words returned, plus 1
    uint32_t buffer[(16 * 4) + 1] = { };

    int num_p = rom_get_partition_table_info(buffer, count_of(buffer), PT_ID_LOCATION_AND_FLAGS);
    if (0 >= num_p || PT_ID_LOCATION_AND_FLAGS != buffer[0]) {
        return PICO_ERROR_GENERIC;
    }

    int picked_p = -1;
    int p = 0;
    int i = 1;

    while (i < num_p) {
        i++;
        uint32_t flags_and_permissions = buffer[i++];
        bool has_id = (flags_and_permissions & PICOBIN_PARTITION_FLAGS_HAS_ID_BITS);
        if (has_id) {
            uint64_t id = 0;
            id |= buffer[i++];
            id |= ((uint64_t)(buffer[i++]) << 32ull);
            if (id == FFS_DATA_PARTITION_ID) {
                picked_p = p;
                break;
            }
        }
        p++;
    }

    if (0 > picked_p) {
        return PICO_ERROR_GENERIC;
    }

    int ret = rom_get_partition_table_info(buffer, count_of(buffer), PT_SINGLE_LOCATION_AND_FLAGS | (picked_p << 24));
    if (3 != ret || PT_SINGLE_LOCATION_AND_FLAGS != buffer[0]) {
        return PICO_ERROR_NOT_FOUND;
    }

    uint32_t loc_and_perms = buffer[1];
    ffs_data.start_offset = ((loc_and_perms >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB) & 0x1fffu) * FLASH_SECTOR_SIZE;
    ffs_data.end_offset = (((loc_and_perms >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB) & 0x1fffu) + 1) * FLASH_SECTOR_SIZE;

    return PICO_OK;
}
#endif // !PICO_RP2040


static uint32_t calc_this_chunk_start(uint32_t offset) {

    uint32_t this_chunk_start = ffs_data.start_offset;

    if (offset >= ffs_data.start_offset + ffs_data.chunk_size
        && offset < ffs_data.end_offset) {  // handle "wrap"
        this_chunk_start += ffs_data.chunk_size;
    }

    return this_chunk_start;
}


static uint32_t calc_next_chunk_start(uint32_t offset) {

    uint32_t next_chunk_start = ffs_data.start_offset;

    if (offset < ffs_data.start_offset + ffs_data.chunk_size) {
        next_chunk_start += ffs_data.chunk_size;
    }

    return next_chunk_start;
}


static int flash_erase_chunk_sectors(uint32_t offset) {

    return rom_flash_op(cflash_erase,
                        FL_OFFSET_TO_ADDR(offset),
                        ffs_data.chunk_size,
                        NULL);
}


static int flash_program_a_page(uint32_t offset, const void *page) {

    return rom_flash_op(cflash_program,
                        FL_OFFSET_TO_ADDR(offset),
                        FLASH_PAGE_SIZE,
                        (uint8_t *)(uintptr_t)page);
}


static int flash_read(uint32_t offset, void *buf, uint32_t length) {

    if (0 == length) {
        return PICO_OK;
    }

    return rom_flash_op(cflash_read,
                        FL_OFFSET_TO_ADDR(offset),
                        length,
                        (uint8_t *)buf);
}


static inline bool flash_read_into_cache(uint32_t offset) {

    return PICO_OK != flash_read(offset, ffs_data.cached_flash_page, FLASH_PAGE_SIZE);
}


// Roll a crc32 over data in flash, reading it in buffer sized pieces
static int flash_crc32_block_of_bytes(uint32_t *crc32, uint32_t offset, uint32_t length) {

    uint8_t buffer[FLASH_READ_BUFFER_SIZE];

    while (0 != length) {
        uint32_t read_len = MIN(length, (uint32_t)sizeof(buffer));

        int ret = flash_read(offset, buffer, read_len);
        if (PICO_OK != ret) {
            return ret;
        }

        *crc32 = crc32_block_of_bytes(*crc32, buffer, read_len);
        offset += read_len;
        length -= read_len;
    }

    return PICO_OK;
}


// Returns true only if the data in flash could be read and matches the supplied data
static bool flash_data_matches(uint32_t offset, const void *data, uint32_t length) {

    const uint8_t *data_ptr = (const uint8_t *)data;
    uint8_t buffer[FLASH_READ_BUFFER_SIZE];

    while (0 != length) {
        uint32_t read_len = MIN(length, (uint32_t)sizeof(buffer));

        if (PICO_OK != flash_read(offset, buffer, read_len) ||
            0 != memcmp(buffer, data_ptr, read_len)) {
            return false;
        }

        data_ptr += read_len;
        offset += read_len;
        length -= read_len;
    }

    return true;
}


static bool flash_write_cache_flush(void) {

    bool error = false;

    // Check if the cache page has data to write in it
    if (NOT_A_VALID_FLASH_OFFSET != ffs_data.wr_page_cache_offset) {
        if (0 > flash_program_a_page(ffs_data.wr_page_cache_offset,
                                     ffs_data.cached_flash_page)) {
            error = true;
        }

        ffs_data.wr_page_cache_offset = NOT_A_VALID_FLASH_OFFSET;
    }

    return error;
}


// NOTE: This function automatically advances the write address used as
// data is written so that callers can easily store consecutive items.
// This function can leave un-programmed data in the write cache which
// can be explicitly written by calling flash_write_cache_flush()
static bool flash_write_via_cache(uint32_t *ptr_to_wr_offset,
                                  const void *data,
                                  uint16_t length) {

    const uint8_t *rd_ptr = (const uint8_t *)data;
    bool error = false;

    while (0 != length && false == error) {

        uint32_t flash_page_start = FL_OFFSET_TO_PAGE_START(*ptr_to_wr_offset);

        // check for a page in ram cache to write into flash
        if (NOT_A_VALID_FLASH_OFFSET == ffs_data.wr_page_cache_offset) {
            if (flash_read_into_cache(flash_page_start)) {
                error = true;
                continue;
            }
            ffs_data.wr_page_cache_offset = flash_page_start;
        }

        // Check if new offset is within the current page
        else if (flash_page_start != ffs_data.wr_page_cache_offset) {
            if (0 > flash_program_a_page(ffs_data.wr_page_cache_offset,
                                         ffs_data.cached_flash_page)) {
                error = true;
            }

            ffs_data.wr_page_cache_offset = NOT_A_VALID_FLASH_OFFSET;
            continue;
        }

        // copy 1 character of data into the flash page holding buffer
        ffs_data.cached_flash_page[*ptr_to_wr_offset % FLASH_PAGE_SIZE] = *rd_ptr++;
        (*ptr_to_wr_offset)++;
        length--;
    }

    return error;
}


// NOTE: This function automatically advances the write address used as
// data is written so that callers can easily store consecutive items.
// This function will program data into flash leaving the cache clear.
static bool flash_write_and_flush_cache(uint32_t *ptr_to_wr_offset,
                                        const void *data,
                                        uint16_t length) {

    bool error = flash_write_via_cache(ptr_to_wr_offset, data, length);

    if (flash_write_cache_flush()) {
        error = true;
    }

    return error;
}


// NOTE: This function automatically advances the write address used as
// data is written so that callers can easily store consecutive items.
// Copies data already in flash, this can leave un-programmed data in the
// write cache which can be explicitly written by calling flash_write_cache_flush()
static bool flash_copy_via_cache(uint32_t *ptr_to_wr_offset,
                                 uint32_t rd_offset,
                                 uint32_t length) {

    uint8_t buffer[FLASH_READ_BUFFER_SIZE];
    bool error = false;

    while (0 != length && false == error) {
        uint16_t read_len = (uint16_t)MIN(length, (uint32_t)sizeof(buffer));

        if (PICO_OK != flash_read(rd_offset, buffer, read_len) ||
            flash_write_via_cache(ptr_to_wr_offset, buffer, read_len)) {
            error = true;
        }

        rd_offset += read_len;
        length -= read_len;
    }

    return error;
}


static bool mark_file_as_deprecated(uint32_t file_offset) {

    uint32_t file_status_offset = file_offset + OFFSET_OF_HDR_STATUS_BYTE();
    const uint8_t status = FILE_STATUS_DEPRECATED;

    return flash_write_and_flush_cache(&file_status_offset, &status, sizeof(status));
}


// Check a file header, which has already been read from flash at `file_offset`.
// The file's data is read from flash (following the header) to check crc0.
static file_found_e check_for_file(ffs_file_hdr_t *pfile_hdr, uint32_t file_offset) {

    if (VIRGIN_FLASH_WORD_VALUE == pfile_hdr->magic[0]) {
        return VIRGIN_FLASH;
    }

    if (0 != memcmp(&pfile_hdr->magic, file_magic, sizeof(file_magic))) {
        return UNEXPECTED_DATA;
    }

    // sanity check length to avoid a broken header possibly bricking the crc check!
    uint32_t data_len = pfile_hdr->data_len;
    if (FFS_API_MAX_WRITE_LENGTH < data_len) {
        return UNEXPECTED_DATA;
    }

    // The data must not extend beyond the ffs flash area
    uint32_t data_offset = file_offset + (uint32_t)sizeof(ffs_file_hdr_t);
    if (data_offset + data_len > ffs_data.end_offset) {
        return UNEXPECTED_DATA;
    }

    // Calculate the crc0 value which comprises the header up
    // until the status field and also the actual file data.
    uint32_t crc0 = file_hdr_calc_crc0(pfile_hdr, NULL, 0);
    if (PICO_OK != flash_crc32_block_of_bytes(&crc0, data_offset, data_len)) {
        return FLASH_READ_ERROR;
    }

    // check crc0, the "residue" should be 0 with an valid header & data, ignoring file status
    if (0 != crc32_block_of_bytes(crc0, &pfile_hdr->crc0, sizeof(pfile_hdr->crc0))) {
        return UNEXPECTED_DATA;
    }

    uint32_t crc1 = file_hdr_calc_crc1(pfile_hdr);

    // check crc1, the "residue" will be 0 for a valid file
    if (0 == crc32_block_of_bytes(crc1, &pfile_hdr->crc1, sizeof(pfile_hdr->crc1))) {
        if (FILE_STATUS_VALID == pfile_hdr->status) {
            return FILE_FOUND_VALID;
        }
    }

    // At this point we have a bad crc1 but still a good crc0
    if (FILE_STATUS_NASCENT == pfile_hdr->status)  {
        return FILE_FOUND_NASCENT;
    }

    if (FILE_STATUS_DEPRECATED == pfile_hdr->status)  {
        return FILE_FOUND_DEPRECATED;
    }

    return UNEXPECTED_DATA;
}


static void *flash_iterator_get_next_file(flash_iterator_t *iterator) {

    while (1) {

        iterator->check_offset += iterator->increment;  // Note: must be 0 first time
        iterator->check_offset = ROUND_UP_TO_NEXT_32B_WORD(iterator->check_offset);

        if (iterator->check_offset >= iterator->end_offset) {
            return NULL;
        }

        // There must be room for a complete file header before the end
        if (iterator->check_offset + sizeof(ffs_file_hdr_t) > iterator->end_offset) {
            return NULL;
        }

        // Note: the caller is returned a pointer to this RAM copy of the header
        ffs_file_hdr_t *pfile_hdr = &iterator->file_header;

        if (PICO_OK != flash_read(iterator->check_offset, pfile_hdr, sizeof(ffs_file_hdr_t))) {
            iterator->file_found = FLASH_READ_ERROR;
            return NULL;
        }

        iterator->file_found = check_for_file(pfile_hdr, iterator->check_offset);

        if (FLASH_READ_ERROR == iterator->file_found) {
            return NULL;
        }

        // Vital: Set increment value for next loop round *before* the (possible) return of file ptr
        if (FILE_FOUND_NASCENT == iterator->file_found ||
            FILE_FOUND_VALID   == iterator->file_found ||
            FILE_FOUND_DEPRECATED == iterator->file_found) {
            // file checked well enough to trust the data length field
            iterator->increment = sizeof(ffs_file_hdr_t) + pfile_hdr->data_len;
        }
        else {
            iterator->increment = 4;
        }

        // If the caller is sufficiently interested, return them the header pointer
        if (iterator->found_filter >= iterator->file_found) {
            return pfile_hdr;
        }
    }
}


// If found, the file's header is copied to `pfile_hdr_out`, if not NULL
static uint search_for_specified_valid_file(uint file_id, ffs_file_hdr_t *pfile_hdr_out) {

    flash_iterator_t iterator;
    iterator.check_offset = ffs_data.start_offset; // All Flash
    iterator.end_offset   = ffs_data.end_offset;
    iterator.increment    = 0;
    iterator.found_filter = FILE_FOUND_VALID;
    ffs_file_hdr_t *pfile_hdr;

    while ((pfile_hdr = flash_iterator_get_next_file(&iterator)) != NULL) {

        if (pfile_hdr->file_id == file_id) {
            if (NULL != pfile_hdr_out) {
                memcpy(pfile_hdr_out, pfile_hdr, sizeof(ffs_file_hdr_t));
            }
            return iterator.check_offset;
        }
    }

    return NOT_A_VALID_FLASH_OFFSET;
}


static int remove_valid_file_if_found(uint file_id) {

    uint existing_file_in_flash = search_for_specified_valid_file(file_id, NULL);

    if (NOT_A_VALID_FLASH_OFFSET == existing_file_in_flash) {
        return PICO_ERROR_NOT_FOUND;
    }

    if (mark_file_as_deprecated(existing_file_in_flash)) {
        return PICO_ERROR_GENERIC;
    }

    return PICO_OK;
}


// The file data is taken from `wrdata` in RAM or, if `wrdata` is NULL,
// copied from existing data in flash at `src_data_offset`.
static bool write_file_to_flash(ffs_file_hdr_t *phdr_buff,
                                const char *wrdata,
                                uint32_t src_data_offset) {

    // The file header structure contains words, it must start on a word boundary
    ffs_data.next_write = ROUND_UP_TO_NEXT_32B_WORD(ffs_data.next_write);

    if (ffs_data.next_write >= ffs_data.end_offset) {
        return true;
    }

    // Step 1: Write file header to flash including crc0 with a 'nascent' status
    uint32_t header_offset = ffs_data.next_write;
    bool error = flash_write_via_cache(&ffs_data.next_write, phdr_buff, sizeof(ffs_file_hdr_t));

    if (NULL != wrdata) {
        if (flash_write_via_cache(&ffs_data.next_write, wrdata, phdr_buff->data_len)) {
            error = true;
        }
    }
    else if (flash_copy_via_cache(&ffs_data.next_write, src_data_offset, phdr_buff->data_len)) {
        error = true;
    }

    // Ensure header and data, along with crc0 are actually *programmed* in flash.
    if (flash_write_cache_flush()) {
        error = true;
    }

    // Step 2 If there is a currently "valid" version of the file, remove it by voiding the record
    (void) remove_valid_file_if_found(phdr_buff->file_id);

    // Step 3: Finalise new file by marking it as 'valid' status / setting crc1
    file_header_validate(phdr_buff);
    if (flash_write_and_flush_cache(&header_offset, phdr_buff, sizeof(ffs_file_hdr_t))) {
        error = true;
    }

    return error;
}


static bool write_new_file_to_flash(uint file_id, const char *wrdata, uint data_len) {

    ffs_file_hdr_t hdr_buff;
    file_header_generate(&hdr_buff, file_id, wrdata, data_len);

    return write_file_to_flash(&hdr_buff, wrdata, NOT_A_VALID_FLASH_OFFSET);
}


static bool copy_file_in_flash(const ffs_file_hdr_t *pfile_hdr, uint32_t file_offset) {

    // The header fields covered by crc0 are unchanged, so crc0 is still
    // correct; just return the copy of the header to the 'nascent' state.
    ffs_file_hdr_t hdr_buff;
    memcpy(&hdr_buff, pfile_hdr, sizeof(ffs_file_hdr_t));
    hdr_buff.status = FILE_STATUS_NASCENT;
    hdr_buff.crc1 = VIRGIN_FLASH_WORD_VALUE;

    uint32_t data_offset = file_offset + (uint32_t)sizeof(ffs_file_hdr_t);

    return write_file_to_flash(&hdr_buff, NULL, data_offset);
}


static bool chunk_erase_and_set_nascent_header(uint32_t offset) {

    bool error = (PICO_OK != flash_erase_chunk_sectors(offset));

    ffs_file_hdr_t hdr_buff;
    file_header_generate(&hdr_buff, FILE_ID_CHUNK_HEADER, NULL, 0);

    if (flash_write_and_flush_cache(&offset, &hdr_buff, sizeof(ffs_file_hdr_t))) {
        error = true;
    }

    return error;
}


static bool chunk_set_header_as_valid(uint32_t offset) {

    ffs_file_hdr_t hdr_buff;

    if (PICO_OK != flash_read(offset, &hdr_buff, sizeof(ffs_file_hdr_t))) {
        return true;
    }

    file_header_validate(&hdr_buff);

    return flash_write_and_flush_cache(&offset, &hdr_buff, sizeof(ffs_file_hdr_t));
}

// Vitally important function:
// 0) Ensure any junk chunks are erased and set as nascent.
// 1) In the case of two nascent chunks, mark the first valid.
// 2) In the case of a deprecated chunk, complete the file copy, erase and set as nascent.
// 3) Return the offset to the start of the valid chunk, or NOT_A_VALID_FLASH_OFFSET on failure.
static uint32_t find_or_make_a_valid_chunk(bool *complete_file_move) {

    *complete_file_move = false;

    file_found_e file_found[NUMBER_OF_CHUNKS];

    // Reading chunk header status - erase and set junk chunks nascent
    uint32_t chunk_offset = ffs_data.start_offset;
    for (int i = 0; i < NUMBER_OF_CHUNKS; i++, chunk_offset += ffs_data.chunk_size) {

        ffs_file_hdr_t file_hdr;
        if (PICO_OK != flash_read(chunk_offset, &file_hdr, sizeof(ffs_file_hdr_t))) {
            return NOT_A_VALID_FLASH_OFFSET;
        }

        file_found[i] = check_for_file(&file_hdr, chunk_offset);

        // Never erase a chunk just because it couldn't be read
        if (FLASH_READ_ERROR == file_found[i]) {
            return NOT_A_VALID_FLASH_OFFSET;
        }

        if (FILE_FOUND_DEPRECATED != file_found[i] &&
            FILE_FOUND_NASCENT != file_found[i] &&
            FILE_FOUND_VALID != file_found[i]) {
            (void)chunk_erase_and_set_nascent_header(chunk_offset);
            file_found[i] = FILE_FOUND_NASCENT;
        }
    }

    if (FILE_FOUND_NASCENT == file_found[0] &&
        FILE_FOUND_NASCENT == file_found[1]) {
        // Special case on boot with empty flash, make chunk 0 valid
        (void)chunk_set_header_as_valid(ffs_data.start_offset);
        return ffs_data.start_offset;
    }

    chunk_offset = ffs_data.start_offset;
    for (int i = 0; i < NUMBER_OF_CHUNKS; i++, chunk_offset += ffs_data.chunk_size) {

        // Handle Valid & Nascent *or* Nascent & Valid
        if (FILE_FOUND_VALID == file_found[i] && FILE_FOUND_NASCENT == file_found[i^1]) {
            return chunk_offset;
        }

        // Handle Valid & Deprecated *or* Deprecated & Valid
        if (FILE_FOUND_VALID == file_found[i] && FILE_FOUND_DEPRECATED == file_found[i^1]) {
            *complete_file_move = true;
            return chunk_offset;
        }
    }

    return NOT_A_VALID_FLASH_OFFSET;
}


// This is an important function, run during initialisation
// Returns NOT_A_VALID_FLASH_OFFSET if the flash could not be read.
static uint32_t get_next_flash_write_offset(uint32_t chunk_start) {

    flash_iterator_t iterator;
    iterator.check_offset = chunk_start;
    iterator.end_offset   = chunk_start + ffs_data.chunk_size;
    iterator.increment    = 0;
    iterator.found_filter = UNEXPECTED_DATA;  // filter for
    ffs_file_hdr_t *pfile_hdr;

    while ((pfile_hdr = flash_iterator_get_next_file(&iterator)) != NULL) {

        if (VIRGIN_FLASH == iterator.file_found) {
            break;  // result!
        }

        if (FILE_FOUND_NASCENT == iterator.file_found && FILE_ID_CHUNK_HEADER != pfile_hdr->file_id) {

            // Remove any "valid" copy, the nascent one will replace this
            (void) remove_valid_file_if_found(pfile_hdr->file_id);

            ffs_file_hdr_t hdr_buff;
            memcpy(&hdr_buff, pfile_hdr, sizeof(ffs_file_hdr_t));

            file_header_validate(&hdr_buff);
            uint32_t finalise_offset = iterator.check_offset; // disposable variable
            (void) flash_write_and_flush_cache(&finalise_offset, &hdr_buff, sizeof(ffs_file_hdr_t));
        }

        else if (UNEXPECTED_DATA == iterator.file_found) {
            //FIXME HERE TODO
        }
    }

    if (FLASH_READ_ERROR == iterator.file_found) {
        return NOT_A_VALID_FLASH_OFFSET;
    }

    return iterator.check_offset;
}


static bool move_existing_files_to_latest_chunk(void) {

    uint error_count = 0;

    // NOTE: Only check the (now) old chunk for files
    uint32_t chunk_to_read_erase = calc_next_chunk_start(ffs_data.next_write);

    flash_iterator_t iterator;
    iterator.check_offset = chunk_to_read_erase;
    iterator.end_offset   = chunk_to_read_erase + ffs_data.chunk_size;
    iterator.increment    = 0;
    iterator.found_filter = FILE_FOUND_VALID; // ONLY valid files
    ffs_file_hdr_t *pfile_hdr;

    while ((pfile_hdr = flash_iterator_get_next_file(&iterator)) != NULL) {

        if (FILE_ID_CHUNK_HEADER == pfile_hdr->file_id) {
            continue; //Do NOT move chunk headers between chunks
        }

        if (copy_file_in_flash(pfile_hdr, iterator.check_offset)) {
            error_count++;
        }
    }

    // Don't erase the old chunk if it couldn't all be read
    if (FLASH_READ_ERROR == iterator.file_found) {
        return true;
    }

    if (chunk_erase_and_set_nascent_header(chunk_to_read_erase)) {
        error_count++;
    }

    if (error_count) {
        return true;
    }

    return false;
}


static uint list_files(file_info_t file_info[], uint max_infos) {

    flash_iterator_t iterator;
    // Only search in the current, active chunk
    iterator.check_offset = calc_this_chunk_start(ffs_data.next_write);
    iterator.end_offset   = iterator.check_offset + ffs_data.chunk_size;
    iterator.increment    = 0;
    iterator.found_filter = FILE_FOUND_VALID; // ONLY valid files

    uint files_found = 0;
    while (files_found < max_infos) {

        ffs_file_hdr_t *pfile_hdr = flash_iterator_get_next_file(&iterator);
        if (NULL == pfile_hdr) {
            break; // no more files!
        }

        // Ignore the chunk header record  - it's not a user accessible r/w file.
        if (FILE_ID_CHUNK_HEADER == pfile_hdr->file_id) {
            continue;
        }

        // Add file details to the list, bump counter.
        file_info[files_found].file_id  = (uint)pfile_hdr->file_id;
        file_info[files_found].data_len = (uint)pfile_hdr->data_len;
        files_found++;
    }

    return files_found;
}


static uint32_t calc_total_size_of_files_except(uint ignore_file_id) {

    flash_iterator_t iterator;
    // Only search in the current, active chunk
    iterator.check_offset = calc_this_chunk_start(ffs_data.next_write);
    iterator.end_offset   = iterator.check_offset + ffs_data.chunk_size;
    iterator.increment    = 0;
    iterator.found_filter = FILE_FOUND_VALID; // ONLY valid files
    ffs_file_hdr_t *pfile_hdr;

    uint32_t file_size_less_one_file = 0;

    // Sum the "in flash" sizes of all files except the specified one
    while ((pfile_hdr = flash_iterator_get_next_file(&iterator)) != NULL) {

        if (ignore_file_id != (uint) pfile_hdr->file_id) {
            file_size_less_one_file += (uint32_t)sizeof(ffs_file_hdr_t);
            file_size_less_one_file += ROUND_UP_TO_NEXT_32B_WORD(pfile_hdr->data_len);
        }
    }

    return file_size_less_one_file;
}

//----------------------------------------------------------------------------

int ffs_initialise(void) {

    ffs_data.initialised = false;
    ffs_data.wr_page_cache_offset = NOT_A_VALID_FLASH_OFFSET;

#ifndef PICO_RP2040
    int status = get_ffs_partition_info();
    if (PICO_OK != status) {
        return status;
    }
#else
    // For now, there are no partitions on RP2040, ffs area is harded coded
    ffs_data.start_offset = FFS_RP2040_FLASH_START_OFFSET;
    ffs_data.end_offset = FFS_RP2040_FLASH_END_OFFSET;
#endif

    // VITAL maths!
    uint32_t ffs_size = ffs_data.end_offset - ffs_data.start_offset;
    ffs_data.chunk_size = ffs_size / 2;

    if (FLASH_SECTOR_SIZE > ffs_data.chunk_size) {
        return PICO_ERROR_BUFFER_TOO_SMALL;
    }

    if ((ffs_data.chunk_size % FLASH_SECTOR_SIZE) ||
        (ffs_data.start_offset % FLASH_SECTOR_SIZE)) {
        return PICO_ERROR_BAD_ALIGNMENT;
    }

    bool complete_file_move;

    uint32_t valid_chunk = find_or_make_a_valid_chunk(&complete_file_move);
    if (NOT_A_VALID_FLASH_OFFSET == valid_chunk) {
        return PICO_ERROR_GENERIC;
    }

    ffs_data.next_write = get_next_flash_write_offset(valid_chunk);
    if (NOT_A_VALID_FLASH_OFFSET == ffs_data.next_write) {
        return PICO_ERROR_GENERIC;
    }

    if (complete_file_move) {
        move_existing_files_to_latest_chunk();
    }

    ffs_data.initialised = true;

    return PICO_OK;
}


// Find a valid file, returning its data length (or PICO_ERROR_xxx)
// and setting the flash offset of its data.
static int locate_valid_file(uint file_id, uint32_t *data_offset) {

    if (!ffs_data.initialised) {
        return PICO_ERROR_INVALID_STATE;
    }

    if (FFS_MAX_VALID_FILE_ID < file_id) {
        return PICO_ERROR_INVALID_ARG;
    }

    ffs_file_hdr_t file_hdr;
    uint32_t file_offset = search_for_specified_valid_file(file_id, &file_hdr);
    if (NOT_A_VALID_FLASH_OFFSET == file_offset) {
        return PICO_ERROR_NOT_FOUND;
    }

    // Data, if any, starts immediately after the file header structure
    *data_offset = file_offset + (uint32_t)sizeof(ffs_file_hdr_t);

    return file_hdr.data_len; // Note: could be 0
}


int ffs_read(uint file_id, char *buf, uint buf_len) {

    if (NULL == buf && 0 != buf_len) {
        return PICO_ERROR_INVALID_ARG;
    }

    uint32_t data_offset;
    int data_len = locate_valid_file(file_id, &data_offset);
    if (0 > data_len) {
        return data_len;
    }

    if (PICO_OK != flash_read(data_offset, buf, MIN((uint)data_len, buf_len))) {
        return PICO_ERROR_GENERIC;
    }

    return data_len;
}


int ffs_delete(uint file_id) {

    if (!ffs_data.initialised) {
        return PICO_ERROR_INVALID_STATE;
    }

    if (FFS_MAX_VALID_FILE_ID < file_id) {
        return PICO_ERROR_INVALID_ARG;
    }

    return remove_valid_file_if_found(file_id);
}


int ffs_write(uint file_id, const char *wrdata, uint data_len) {

    bool error = false;

    if (!ffs_data.initialised) {
        return PICO_ERROR_INVALID_STATE;
    }

    if (NULL == wrdata) {
        return PICO_ERROR_INVALID_ARG;
    }

    if (FFS_MAX_VALID_FILE_ID < file_id) {
        return PICO_ERROR_INVALID_ARG;
    }

    if (FFS_API_MAX_WRITE_LENGTH < data_len) {
        return PICO_ERROR_INVALID_ARG;
    }

    bool mv_files_to_new_chunk = false;

    uint32_t new_filesize = (uint32_t)sizeof(ffs_file_hdr_t) + ROUND_UP_TO_NEXT_32B_WORD(data_len);
    uint32_t end_offset = ROUND_UP_TO_NEXT_32B_WORD(ffs_data.next_write + new_filesize);
    uint32_t current_chunk = calc_this_chunk_start(ffs_data.next_write);
    uint32_t chunk_after_write = calc_this_chunk_start(end_offset);

    if (chunk_after_write != current_chunk) {
        // New chunk: First check we can keep all existing (valid) files.
        // The extra +4 is vital to avoid round-up causing an unhandled wrap.
        uint32_t current_size = calc_total_size_of_files_except(file_id);
        if (ffs_data.chunk_size < current_size + new_filesize + 3) {
            return PICO_ERROR_INSUFFICIENT_RESOURCES;
        }

        // Size ok, prepare to write in the new chunk
        uint32_t next_write = get_next_flash_write_offset(chunk_after_write);
        if (NOT_A_VALID_FLASH_OFFSET == next_write) {
            return PICO_ERROR_GENERIC;
        }

        ffs_data.next_write = next_write;
        mv_files_to_new_chunk = true;

        // mark current (soon to be old) chunk as deprecated
        if (mark_file_as_deprecated(current_chunk)) {
            error = true;
        }

        // mark new chunk as valid
        if (chunk_set_header_as_valid(chunk_after_write)) {
            error = true;
        }
    }

    if (write_new_file_to_flash(file_id, wrdata, data_len)) {
        error = true;
    }

    if (mv_files_to_new_chunk) {
        if (move_existing_files_to_latest_chunk()) {
            error = true;
        }
    }

    if (error) {
        return PICO_ERROR_GENERIC;
    }

    return PICO_OK;
}


int ffs_list(file_info_t file_info[], uint max_infos) {

    if (!ffs_data.initialised) {
        return PICO_ERROR_INVALID_STATE;
    }

    if (NULL == file_info) {
        return PICO_ERROR_INVALID_ARG;
    }

    return (int) list_files(file_info, max_infos);
}


char *ffs_get_string(uint8_t file_id) {
    uint32_t data_offset;
    int data_len = locate_valid_file(file_id, &data_offset);
    if (0 > data_len) {
        return NULL;
    }

    char *str = malloc((size_t)data_len + 1);
    if (NULL == str) {
        return NULL;
    }

    if (PICO_OK != flash_read(data_offset, str, (uint32_t)data_len)) {
        free(str);
        return NULL;
    }

    str[data_len] = '\0';
    return str;
}


// Updates the FFS file data if the new value is different from the existing value.
// If the file does not exist, it is created.
int ffs_update_string(uint8_t file_id, const char *data) {
    uint data_len = strlen(data) + 1; // Include the null terminator
    uint32_t data_offset;
    int rc = locate_valid_file(file_id, &data_offset);
    if (rc >= 0 && (uint)rc == data_len) {
        if (flash_data_matches(data_offset, data, data_len))
            return PICO_OK;
    }

    return ffs_write(file_id, data, data_len);
}