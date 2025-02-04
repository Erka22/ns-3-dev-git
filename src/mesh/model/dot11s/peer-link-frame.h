/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef PEER_LINK_FRAME_START_H
#define PEER_LINK_FRAME_START_H
#include "dot11s-mac-header.h"
#include "ie-dot11s-configuration.h"
#include "ie-dot11s-id.h"
#include "ie-dot11s-peering-protocol.h"

#include "ns3/header.h"
#include "ns3/supported-rates.h"

#include <optional>

namespace ns3
{
namespace dot11s
{
/**
 * @ingroup dot11s
 *
 * @brief 802.11s Peer link open management frame
 *
 * Peer link open start frame includes the following:
 * - Capability
 * - Supported rates
 * - Mesh ID of mesh
 * - Configuration
 */
class PeerLinkOpenStart : public Header
{
  public:
    PeerLinkOpenStart();

    // Delete copy constructor and assignment operator to avoid misuse
    PeerLinkOpenStart(const PeerLinkOpenStart&) = delete;
    PeerLinkOpenStart& operator=(const PeerLinkOpenStart&) = delete;

    /// @brief fields:
    struct PlinkOpenStartFields
    {
        IePeeringProtocol protocol; ///< Peering protocol version - 3 octets
        uint16_t capability;        ///< open and confirm
        SupportedRates rates;       ///< open and confirm
        std::optional<ExtendedSupportedRatesIE> extendedRates; ///< open and confirm
        IeMeshId meshId;                                       ///< open and close
        IeConfiguration config;                                ///< open and confirm
    };

    /**
     * Set peer link open start fields
     * @param fields PlinkOpenStartFields to set
     */
    void SetPlinkOpenStart(PlinkOpenStartFields fields);
    /**
     * Get peer link open start fields
     * @return PlinkOpenStartFields
     */
    PlinkOpenStartFields GetFields() const;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    // Inherited from header:
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint16_t m_capability;                                   ///< capability
    SupportedRates m_rates;                                  ///< rates
    std::optional<ExtendedSupportedRatesIE> m_extendedRates; ///< extended rates
    IeMeshId m_meshId;                                       ///< mesh ID
    IeConfiguration m_config;                                ///< config

    /**
     * equality operator
     *
     * @param a lhs
     * @param b rhs
     * @returns true if equal
     */
    friend bool operator==(const PeerLinkOpenStart& a, const PeerLinkOpenStart& b);
};

bool operator==(const PeerLinkOpenStart& a, const PeerLinkOpenStart& b);

/**
 * @ingroup dot11s
 *
 * @brief 802.11s Peer link close management frame
 *
 * Peer link close frame includes the following:
 * - Mesh ID of mesh
 */
class PeerLinkCloseStart : public Header
{
  public:
    PeerLinkCloseStart();

    // Delete copy constructor and assignment operator to avoid misuse
    PeerLinkCloseStart(const PeerLinkCloseStart&) = delete;
    PeerLinkCloseStart& operator=(const PeerLinkCloseStart&) = delete;

    /// @brief fields:
    struct PlinkCloseStartFields
    {
        IePeeringProtocol protocol; ///< Peering protocol version - 3 octets
        IeMeshId meshId;            ///< open and close
    };

    /**
     * Set peer link close start fields
     * @param fields PlinkCloseStartFields to set
     */
    void SetPlinkCloseStart(PlinkCloseStartFields fields);
    /**
     * Get peer link close start fields
     * @return PlinkOpenStartFields
     */
    PlinkCloseStartFields GetFields() const;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    // Inherited from header:
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    IeMeshId m_meshId; ///< mesh ID

    /**
     * equality operator
     *
     * @param a lhs
     * @param b rhs
     * @returns true if equal
     */
    friend bool operator==(const PeerLinkCloseStart& a, const PeerLinkCloseStart& b);
};

bool operator==(const PeerLinkCloseStart& a, const PeerLinkCloseStart& b);

/**
 * @ingroup dot11s
 *
 * @brief 802.11s Peer link confirm management frame
 *
 * Peer link confirm frame includes the following:
 * - Association ID field
 * - Supported rates
 * - Configuration
 */
class PeerLinkConfirmStart : public Header
{
  public:
    PeerLinkConfirmStart();

    // Delete copy constructor and assignment operator to avoid misuse
    PeerLinkConfirmStart(const PeerLinkConfirmStart&) = delete;
    PeerLinkConfirmStart& operator=(const PeerLinkConfirmStart&) = delete;

    /// @brief fields:
    struct PlinkConfirmStartFields
    {
        IePeeringProtocol protocol; ///< Peering protocol version - 3 octets
        uint16_t capability;        ///< open and confirm
        uint16_t aid;               ///< confirm only
        SupportedRates rates;       ///< open and confirm
        std::optional<ExtendedSupportedRatesIE> extendedRates; ///< open and confirm
        IeConfiguration config;                                ///< open and confirm
    };

    /**
     * Set peer link confirm start fields
     * @param fields PlinkCloseStartFields to set
     */
    void SetPlinkConfirmStart(PlinkConfirmStartFields fields);
    /**
     * Get peer link confirm start fields
     * @return PlinkOpenStartFields
     */
    PlinkConfirmStartFields GetFields() const;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    // Inherited from header:
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint16_t m_capability;                                   ///< capability
    uint16_t m_aid;                                          ///< aid
    SupportedRates m_rates;                                  ///< rates
    std::optional<ExtendedSupportedRatesIE> m_extendedRates; ///< extended rates
    IeConfiguration m_config;                                ///< config

    /**
     * equality operator
     *
     * @param a lhs
     * @param b rhs
     * @returns true if equal
     */
    friend bool operator==(const PeerLinkConfirmStart& a, const PeerLinkConfirmStart& b);
};

bool operator==(const PeerLinkConfirmStart& a, const PeerLinkConfirmStart& b);
} // namespace dot11s
} // namespace ns3
#endif
