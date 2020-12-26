#include <eosio/eosio.hpp>

// Message table
struct [[eosio::table("message"), eosio::contract("talk")]] message {
    uint64_t    id       = {}; // Non-0
    uint64_t    reply_to = {}; // Non-0 if this is a reply
    eosio::name user     = {};
    std::string content  = {};

    uint64_t primary_key() const { return id; }
    uint64_t get_reply_to() const { return reply_to; }
};

using message_table = eosio::multi_index<
    "message"_n, message, eosio::indexed_by<"by.reply.to"_n, eosio::const_mem_fun<message, uint64_t, &message::get_reply_to>>
    >;

// Likes table
struct [[eosio::table("likes"), eosio::contract("talk")]] likes {
    uint64_t    id       = {}; // Non-0
    uint64_t    reply_to = {}; // Non-0 if this is a reply
    eosio::name user     = {};

    uint64_t primary_key() const { return id; }
    uint64_t get_reply_to() const { return reply_to; }
    eosio::name get_user() const { return user; }
};
using likes_table = eosio::multi_index<
    "likes"_n, likes, 
    eosio::indexed_by<"by.reply.to"_n, eosio::const_mem_fun<likes, uint64_t, &likes::get_reply_to>>
    // eosio::indexed_by<"by.user"_n, eosio::const_mem_fun<likes, eosio::name, &likes::get_user>>
    >;

// The contract
class talk : eosio::contract {
  public:
    // Use contract's constructor
    using contract::contract;

    // Post a message
    [[eosio::action]] void post(uint64_t id, uint64_t reply_to, eosio::name user, const std::string& content) {
        message_table table{get_self(), 0};
        // Check user
        require_auth(user);

        // Check reply_to exists
        if (reply_to)
            table.get(reply_to);

        // Create an ID if user didn't specify one
        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");
        if (!id)
            id = std::max(table.available_primary_key(), 1'000'000'000ull);

        // Record the message
        table.emplace(get_self(), [&](auto& message) {
            message.id       = id;
            message.reply_to = reply_to;
            message.user     = user;
            message.content  = content;
        });
    }
    // Like a message
    [[eosio::action]] void like(uint64_t id, uint64_t reply_to, eosio::name user) {
        likes_table table{get_self(), 0};
        // Check user
        require_auth(user);

        message_table msg_table{get_self(), 0};

        // Check reply_to exists
        if (reply_to)
            msg_table.get(reply_to);
        // find user and reply_to 
        bool found = false;
        for(auto itr = table.begin(); itr != table.end() && found!=true;) {
            if(itr->user == user && reply_to == itr->reply_to) {
                found = true;
                return;
            }      
        }
        // Create an ID if user didn't specify one
        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");
        if (!id)
            id = std::max(table.available_primary_key(), 1'000'000'000ull);

        // Record the message
        table.emplace(get_self(), [&](auto& like) {
            like.id       = id;
            like.reply_to = reply_to;
            like.user     = user;
        });
    }
    // unlike a message
    [[eosio::action]] void unlike(uint64_t reply_to, eosio::name user) {
        likes_table table{get_self(), 0};
        // Check user
        require_auth(user);

        message_table msg_table{get_self(), 0};

        // Check reply_to exists
        if (reply_to)
            msg_table.get(reply_to);
        // find user and reply_to 
        bool found = false;
        for(auto itr = table.begin(); itr != table.end() && found!=true;) {
            if(itr->user == user && reply_to == itr->reply_to) {
                found = true;
                table.erase( itr );
                return;
            }      
        }
        // eosio::check(found, "haven't found like, need like before unlike!");
    }
};
