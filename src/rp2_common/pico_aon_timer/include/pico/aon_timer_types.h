/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_AON_TIMER_TYPES_H
#define _PICO_AON_TIMER_TYPES_H

#include "pico.h"
#include "pico/time.h"

/*! \typedef aon_timer_time_t
 *  \brief An opaque 64 bit timestamp in milliseconds
 *
 * The type is used instead of a raw uint64_t to prevent accidentally passing relative times or times in the wrong
 * time units where an aon_timer_time_t is required.
 *
 * note: This type defaults to being a uin64_t (i.e. no protection); protection is enabled
 * by setting PICO_OPAQUE_ABSOLUTE_TIME_T to 1
 *
 * \see aon_timer_time_to_ms()
 * \see aon_timer_time_update_ms()
 * \ingroup aon_timer
 */
#if PICO_OPAQUE_ABSOLUTE_TIME_T
typedef struct {
    uint64_t _private_ms;
} aon_timer_time_t;
#else
typedef uint64_t aon_timer_time_t;
#endif

/*! fn aon_timer_time_to_ms
 * \brief convert an aon_timer_time_t into a number of milliseconds.
 * \param t the aon_timer_time_t to convert
 * \return a number of milliseconds, equivalent to t
 * \ingroup aon_timer
 */
 static inline uint64_t aon_timer_time_to_ms(aon_timer_time_t t) {
#if PICO_OPAQUE_ABSOLUTE_TIME_T
    return t._private_ms;
#else
    return t;
#endif
}

/*! fn aon_timer_time_update_ms
    * \brief update an aon_timer_time_t value to represent a given number of milliseconds
    * \param t the aon_timer_time_t value to update
    * \param ms the number of milliseconds to represent. Note this should be representable
    *                      as a signed 64 bit integer
    * \ingroup aon_timer
    */
static inline void aon_timer_time_update_ms(aon_timer_time_t *t, uint64_t ms) {
#if PICO_OPAQUE_ABSOLUTE_TIME_T
    assert(ms <= INT64_MAX);
    t->_private_ms = ms;
#else
    *t = ms;
#endif
}

/*! fn aon_timer_time_from_ms
    * \brief convert a number of milliseconds to an aon_timer_time_t
    * \param ms number of milliseconds
    * \return an aon_timer_time_t equivalent to ms
    * \ingroup aon_timer
    */
static inline aon_timer_time_t aon_timer_time_from_ms(uint64_t ms) {
    aon_timer_time_t t;
    aon_timer_time_update_ms(&t, ms);
    return t;
}

/*! \brief Return a aon_timer_time_t value obtained by adding a number of milliseconds to another aon_timer_time_t
 * \ingroup aon_timer
 *
 * \param t the base aon_timer_time_t
 * \param ms the number of milliseconds to add
 * \return the aon_timer_time_t representing the resulting time
 */
static inline aon_timer_time_t aon_timer_delayed_by_ms(const aon_timer_time_t t, uint32_t ms) {
    uint64_t base = aon_timer_time_to_ms(t);
    uint64_t delayed = base + ms;
    if ((int64_t)delayed < 0) {
        // aon_timer_time_t (to allow for signed time deltas) is never greater than INT64_MAX which == at_the_end_of_time
        delayed = INT64_MAX;
    }
    return aon_timer_time_from_ms(delayed);
}

/*! \brief Return the difference in milliseconds between two aon_timer_time_t values
 * \ingroup aon_timer
 *
 * \param from the first aon_timer_time_t
 * \param to the second aon_timer_time_t
 * \return the number of milliseconds between the two timestamps (positive if `to` is after `from` except
 * in case of overflow)
 */
static inline int64_t aon_timer_time_diff_ms(aon_timer_time_t from, aon_timer_time_t to) {
    return (int64_t)(aon_timer_time_to_ms(to) - aon_timer_time_to_ms(from));
}

/*! fn aon_timer_time_to_absolute_time
 * \brief convert an aon_timer_time_t to an absolute_time_t
 * \param t the aon_timer_time_t to convert
 * \return an absolute_time_t equivalent to t
 * \ingroup aon_timer
 */
static inline absolute_time_t aon_timer_time_to_absolute_time(aon_timer_time_t t) {
    return from_us_since_boot(aon_timer_time_to_ms(t) * 1000);
}

/*! fn absolute_time_to_aon_timer_time
 * \brief convert an absolute_time_t to an aon_timer_time_t
 * \param t the absolute_time_t to convert
 * \return an aon_timer_time_t equivalent to t
 * \ingroup aon_timer
 */
static inline aon_timer_time_t absolute_time_to_aon_timer_time(absolute_time_t t) {
    return aon_timer_time_from_ms(to_us_since_boot(t) / 1000);
}

#endif