/*
 * This file is part of the Soletta Project
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdbool.h>

#include <sol-common-buildopts.h>
#include <sol-vector.h>
#include <sol-str-slice.h>
#include <sol-buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These are routines that Soletta provides for handling network
 * link interfaces, making it possible to observe events,
 * to inquire available links and to set their states.
 */

/**
 * @defgroup Comms Communication Modules
 *
 * @brief Comms consists on a few communication modules.
 *
 * It provides ways to deal with network, CoAP protocol and
 * OIC protocol (server and client sides).
 *
 * @defgroup Network Network
 * @ingroup Comms
 *
 * @brief Network module provides a way to handle network link interfaces.
 *
 * It makes possible to observe events, to inquire available links
 * and to set their states.
 *
 * @{
 */

/**
 * @brief String size of an IPv4/v6 address.
 */
#define SOL_INET_ADDR_STRLEN 48


/**
 * @struct sol_network_hostname_handle
 *
 * @brief A handle to sol_network_get_hostname_address_info()
 *
 * This handle can be used to cancel get sol_network_get_hostname_address_info()
 * by calling sol_network_cancel_get_hostname_address_info()
 *
 * @see sol_network_get_hostname_address_info()
 * @see sol_network_cancel_get_hostname_address_info()
 */
struct sol_network_hostname_handle;

/**
 * @brief Type of events generated for a network link.
 *
 * @see sol_network_subscribe_events()
 */
enum sol_network_event {
    SOL_NETWORK_LINK_ADDED,
    SOL_NETWORK_LINK_REMOVED,
    SOL_NETWORK_LINK_CHANGED,
};

/**
 * @brief Bitwise OR-ed flags to represents the status of #sol_network_link.
 *
 * @see sol_network_link
 */
enum sol_network_link_flags {
    SOL_NETWORK_LINK_UP            = (1 << 0),
    SOL_NETWORK_LINK_BROADCAST     = (1 << 1),
    SOL_NETWORK_LINK_LOOPBACK      = (1 << 2),
    SOL_NETWORK_LINK_MULTICAST     = (1 << 3),
    SOL_NETWORK_LINK_RUNNING       = (1 << 4),
};

/**
 * @brief Type of a network address
 *
 * Tells how an address should be interpreted.
 */
enum sol_network_family {
    /** @brief Unspecified address type */
    SOL_NETWORK_FAMILY_UNSPEC,
    /** @brief IPv4 family. */
    SOL_NETWORK_FAMILY_INET,
    /** @brief IPv6 family. */
    SOL_NETWORK_FAMILY_INET6
};

/**
 * @brief Structure to represent a network address, both IPv6 and IPv4 are valid.
 */
struct sol_network_link_addr {
    enum sol_network_family family; /**< @brief IPv4 or IPv6 family */
    union {
        uint8_t in[4];
        uint8_t in6[16];
    } addr; /**< @brief The address itself */
    uint16_t port; /**< @brief The port associed with the IP address */
};

/**
 * @brief Structure to represent a network link.
 *
 * This struct contains the necessary information do deal with a
 * network link. It has the state @ref sol_network_link_flags, the
 * index (the value used by the SO to identify the link) and its
 * address @ref sol_network_link_addr.
 */
struct sol_network_link {
#ifndef SOL_NO_API_VERSION
#define SOL_NETWORK_LINK_API_VERSION (1)
    uint16_t api_version; /**< @brief API version */
#endif
    uint16_t index; /**< @brief the index of this link given by SO  */
    enum sol_network_link_flags flags; /**< @brief  The status of the link */
    /**
     * @brief List of network addresses.
     * @see sol_network_link_addr
     **/
    struct sol_vector addrs;
};

#ifndef SOL_NO_API_VERSION
/**
 * @brief Macro used to check if a struct @c struct sol_network_link has
 * the expected API version.
 *
 * In case it has wrong version, it'll return extra arguments passed
 * to the macro.
 */
#define SOL_NETWORK_LINK_CHECK_VERSION(link_, ...) \
    if (SOL_UNLIKELY((link_)->api_version != \
        SOL_NETWORK_LINK_API_VERSION)) { \
        SOL_WRN("Unexpected API version (message is %u, expected %u)", \
            (link_)->api_version, SOL_NETWORK_LINK_API_VERSION); \
        return __VA_ARGS__; \
    }
#else
#define SOL_NETWORK_LINK_CHECK_VERSION(link_, ...)
#endif

/**
 * @brief Converts a @c sol_network_link_addr to a string.
 *
 * @param addr The address to be converted.
 * @param buf The buffer where the converted string will be appended -
 * It must be already initialized.
 *
 * @return a string with the network link address on success, @c NULL on error.
 *
 * @see sol_network_link_addr_from_str()
 */
const char *sol_network_link_addr_to_str(const struct sol_network_link_addr *addr, struct sol_buffer *buf);

/**
 * @brief Converts a string address to @c sol_network_link_addr.
 *
 * @param addr A valid address with the same family of the address given in @c buf.
 * @param buf The string with the address.
 *
 * @return the network link address on success, @c NULL on error.
 *
 * @see sol_network_link_addr_to_str()
 */
const struct sol_network_link_addr *sol_network_link_addr_from_str(struct sol_network_link_addr *addr, const char *buf);

/**
 * @brief Checks if two address are equal.
 *
 * This function compares two addresses to see if they are the same.
 *
 * @param a The first address to be compared.
 * @param b The second address to be compared.
 *
 * @return @c true if they are equal, otherwise @c false.
 */
static inline bool
sol_network_link_addr_eq(const struct sol_network_link_addr *a,
    const struct sol_network_link_addr *b)
{
    const uint8_t *addr_a, *addr_b;
    size_t bytes;

    if (a->family != b->family)
        return false;

    if (a->family == SOL_NETWORK_FAMILY_INET) {
        addr_a = a->addr.in;
        addr_b = b->addr.in;
        bytes = sizeof(a->addr.in);
    } else if (a->family == SOL_NETWORK_FAMILY_INET6) {
        addr_a = a->addr.in6;
        addr_b = b->addr.in6;
        bytes = sizeof(a->addr.in6);
    } else
        return false;

    return !memcmp(addr_a, addr_b, bytes);
}

/**
 * @brief Subscribes on to receive network link events.
 *
 * This function register a callback given by the user that will be
 * called when a network event (@ref sol_network_event) occurrs in one
 * link (@ref sol_network_link).
 *
 * @param cb The callback used to notify the user.
 * @param data The user data given in the callback.
 *
 * @return @c true on success, @c false on error
 *
 * @see sol_network_unsubscribe_events()
 */
bool sol_network_subscribe_events(void (*cb)(void *data, const struct sol_network_link *link,
    enum sol_network_event event),
    const void *data);

/**
 * @brief Stops receive the network events.
 *
 * This function removes previous callbacks set (@ref
 * sol_network_subscribe_events()) to receive network events.
 *
 * @param cb The callback given on @ref sol_network_subscribe_events.
 * @param data The data given on @ref sol_network_subscribe_events.
 *
 * @return @c true on success, @c false on error
 *
 * @note It should be the same pair (callback/userdata) given on @ref
 * sol_network_subscribe_events()
 */
bool sol_network_unsubscribe_events(void (*cb)(void *data, const struct sol_network_link *link,
    enum sol_network_event event),
    const void *data);
/**
 * @brief Retrieve the available network links on system.
 *
 * @return A vector containing the available links @see sol_network_link
 *
 * @note This vector is updated as soon as the SO notifies about a
 * network link. This is information is cached, so it's possible that
 * at the moment it is called the data is still not available. It's
 * recommended first subscribe to receive network events @see
 * sol_network_subscribe_events() and then call it.
 */
const struct sol_vector *sol_network_get_available_links(void);

/**
 * @brief Gets the name of a network link.
 *
 * @param link The @ref sol_network_link structure which the name is desired.
 * @return The name of the interface on success, @c NULL on error.
 */
char *sol_network_link_get_name(const struct sol_network_link *link);

/**
 * @brief Sets a network link up.
 *
 * This function sets a network link up, after this a link will be
 * able to get a network address.
 *
 * @param link_index The index of a @ref sol_network_link structure.
 * @return @c true on success, @c false on error.
 *
 * @see sol_network_linke_down()
 */
bool sol_network_link_up(uint16_t link_index);

/**
 * @brief Sets a network link down.
 *
 * This function sets a network link down, after this a link will not be
 * able to get a network address.
 *
 * @param link_index The index of a @ref sol_network_link structure.
 * @return @c true on success, @c false on error.
 *
 * @see sol_network_linke_up()
 */
bool sol_network_link_down(uint16_t link_index);

/**
 * @brief Gets a hostname address info.
 *
 * This function will fetch the address of a given hostname, since this may
 * take some time, this will be an async operation. When the address info
 * is ready the @c host_info_cb will called with the host's address info.
 * If an error happens or it was not possible to fetch the host address
 * information, @c addrs_list will be set to @c NULL.
 * The list @c addrs_list will contains a set of #sol_network_link_addr.
 *
 * @param hostname The hostname to get the address info.
 * @param family The family the returned addresses should be, pass SOL_NETWORK_FAMILY_UNSPEC
 * to match them all.
 * @param host_info_cb A callback to be called with the address list.
 * @param data Data to @c host_info_cb.
 * @return A handle to a hostname or @c NULL on error.
 * @see sol_network_cancel_get_hostname_address_info()
 * @see #sol_network_family
 */
struct sol_network_hostname_handle *
sol_network_get_hostname_address_info(const struct sol_str_slice hostname,
    enum sol_network_family family, void (*host_info_cb)(void *data,
    const struct sol_str_slice hostname, const struct sol_vector *addrs_list),
    const void *data);

/**
 * @brief Cancels a request to get the hostname info.
 *
 * @param handle The handle returned by #sol_network_get_hostname_address_info
 * @return 0 on success, -errno on error.
 * @see sol_network_get_hostname_address_info()
 */
int sol_network_cancel_get_hostname_address_info(struct sol_network_hostname_handle *handle);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
