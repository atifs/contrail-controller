/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __BGP_SERVER_TEST_UTIL_H__
#define __BGP_SERVER_TEST_UTIL_H__

#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>

#include "base/util.h"
#include "base/test/task_test_util.h"
#include "bgp/bgp_config.h"
#include "bgp/bgp_log.h"
#include "bgp/bgp_peer.h"
#include "bgp/bgp_peer_types.h"
#include "bgp/bgp_server.h"
#include "bgp/bgp_peer_close.h"
#include "bgp/routing-instance/routing_instance.h"

#include "db/db.h"
#include "db/db_graph.h"

class BgpPeerTest;

class BgpInstanceConfigTest : public BgpInstanceConfig {
public:
    BgpInstanceConfigTest(const std::string &name) : BgpInstanceConfig(name) {
    }
    RouteTargetList *mutable_import_list() { return &import_list_; }
    RouteTargetList *mutable_export_list() { return &export_list_; }
};

class BgpTestUtil {
public:
    static std::auto_ptr<BgpInstanceConfigTest> CreateBgpInstanceConfig(
            const std::string &name, const std::string import_targets,
            const std::string export_targets);

    void SetUserData(std::string key, boost::any &value);
    boost::any GetUserData(std::string key);

private:
    std::map<std::string, boost::any> user_data_;
};

class BgpServerTest : public BgpServer {
public:
    BgpServerTest(EventManager *evm, const std::string &localname,
                  DB *config_db, DBGraph *config_graph);
    BgpServerTest(EventManager *evm, const std::string &localname);
    ~BgpServerTest();
    bool Configure(const std::string &config);
    BgpPeerTest *FindPeerByUuid(const char *routing_instance,
                                const std::string &uuid);
    BgpPeer *FindPeer(const char *routing_instance,
                      const std::string &peername);
    void Shutdown();

    DB *config_db() { return config_db_.get(); }
    DBGraph *config_graph() { return config_graph_.get(); }

    static void GlobalSetUp();
    void set_autonomous_system(as_t as) { autonomous_system_ = as; }
    void set_bgp_identifier(uint32_t bgp_id) {
        Ip4Address addr(bgp_id);
        bgp_identifier_ = addr;
    }

    virtual std::string ToString() const;
    virtual bool IsPeerCloseGraceful() {
        return GetIsPeerCloseGraceful_fnc_();
    }

    bool BgpServerIsPeerCloseGraceful() {
        return BgpServer::IsPeerCloseGraceful();
    }

    boost::function<bool()> GetIsPeerCloseGraceful_fnc_;

private:
    void PostShutdown();

    std::string name_;
    boost::scoped_ptr<DB> config_db_;
    boost::scoped_ptr<DBGraph> config_graph_;
    bool cleanup_config_;
};

class BgpPeerTest : public BgpPeer {
public:
    BgpPeerTest(BgpServer *server, RoutingInstance *rtinst,
                const BgpNeighborConfig *config);
    ~BgpPeerTest();

    virtual void StartKeepaliveTimerUnlocked();
    void BindLocalEndpoint(BgpSession *session);

    static void verbose_name(bool verbose) { verbose_name_ = verbose; }
    std::string ToString() const;

    bool BgpPeerSendUpdate(const uint8_t *msg, size_t msgsize);
    virtual bool SendUpdate(const uint8_t *msg, size_t msgsize) {
        return SendUpdate_fnc_(msg, msgsize);
    }

    bool BgpPeerMpNlriAllowed(uint16_t afi, uint8_t safi);
    virtual bool MpNlriAllowed(uint16_t afi, uint8_t safi) {
        return MpNlriAllowed_fnc_(afi, safi);
    }
  
    bool BgpPeerIsReady();
    void SetDataCollectionKey(BgpPeerInfo *peer_info) const;

    virtual bool IsReady() const {
        return IsReady_fnc_();
    }

    boost::function<bool(const uint8_t *, size_t)> SendUpdate_fnc_;
    boost::function<bool(uint16_t, uint8_t)> MpNlriAllowed_fnc_;
    boost::function<bool()> IsReady_fnc_;

    BgpTestUtil util_;

private:
    static bool verbose_name_;
};

class RoutingInstanceTest : public RoutingInstance {
public:
    RoutingInstanceTest(std::string name, BgpServer *server,
                        RoutingInstanceMgr *mgr,
                        const BgpInstanceConfig *config);
    virtual BgpPeer *PeerLocate(BgpServer *server,
                                const BgpNeighborConfig *config);
    virtual BgpPeer *PeerLookup(boost::asio::ip::tcp::endpoint remote_endpoint);
    virtual void DestroyIPeer(IPeer *ipeer);
private:
    typedef std::map<boost::uuids::uuid, BgpPeer *> PeerByUuidMap;
    PeerByUuidMap peers_by_uuid_;
};

#define BGP_WAIT_FOR_PEER_STATE(peer, state)                                   \
    TASK_UTIL_WAIT_EQ(state, (peer)->GetState(), task_util_wait_time(),        \
                      task_util_retry_count(), "Peer State")

#define BGP_WAIT_FOR_PEER_DELETION(peer)  \
    TASK_UTIL_EXPECT_EQ_MSG(NULL, peer, "Peer Deletion")

#define BGP_VERIFY_ROUTE_COUNT(table, count)                                   \
    TASK_UTIL_EXPECT_EQ_MSG(count, static_cast<int>((table)->Size()),          \
                            "Wait for route count")

#define BGP_VERIFY_ROUTE_PRESENCE(table, route) \
    TASK_UTIL_EXPECT_NE_MSG(static_cast<BgpRoute *>(NULL),                     \
                            (table)->Find(route), "Route Presence")

#define BGP_VERIFY_ROUTE_ABSENCE(table, route) \
    TASK_UTIL_EXPECT_EQ_MSG(static_cast<BgpRoute *>(NULL),                     \
                            (table)->Find(route), "Route Absence")

#endif // __BGP_SERVER_TEST_UTIL_H__