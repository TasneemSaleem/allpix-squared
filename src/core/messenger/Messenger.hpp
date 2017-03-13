/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MESSENGER_H
#define ALLPIX_MESSENGER_H

#include <vector>
#include <map>
#include <typeindex>

#include "delegates.h"
#include "../module/Module.hpp"
#include "Message.hpp"

namespace allpix {

    class Messenger {
    public:
        // Constructor and destructors
        Messenger();
        virtual ~Messenger();
        
        // Disallow copy
        Messenger(const Messenger&) = delete;
        Messenger &operator=(const Messenger&) = delete;
        
        // FIXME: unregister of listeners still need to be added...
        
        // Register a listener
        // FIXME: is the empty string a proper catch-all or do we want to have a clearer separation?
        template <typename T, typename R> void registerListener(T *receiver, void (T::*)(std::shared_ptr<R>), std::string message_type = "");
        
        // Register a single bind
        // FIXME: better names?
        template <typename T, typename R> void bindSingle(T *receiver, std::shared_ptr<R> T::*, std::string message_type = "");
        template <typename T, typename R> void bindMulti(T *receiver, std::vector<std::shared_ptr<R>> T::*, std::string message_type = "");
        
        // Dispatch message
        template <typename T> void dispatchMessage(const T &msg, std::string name = "");
        template <typename T> void dispatchMessage(std::shared_ptr<T> msg, std::string name = "");
        void dispatchMessage(std::shared_ptr<Message> msg, std::string name = "");
    private:
        using DelegateMap = std::map<std::type_index, std::map<std::string, std::vector<BaseDelegate*>>>;
        
        DelegateMap delegates_;
    };
    
    // dispatch a message from a reference
    // WARNING: provide this option or not (leads to copying)
    template <typename T> void Messenger::dispatchMessage(const T &msg, std::string name) {
        static_assert(std::is_base_of<Message, T>::value, "Dispatched message should inherit from Message class");
        std::shared_ptr<Message> msg_ptr = std::make_shared<T>(msg);
        dispatchMessage(msg_ptr, name);
    }
    template <typename T> void Messenger::dispatchMessage(std::shared_ptr<T> msg, std::string name) {
        static_assert(std::is_base_of<Message, T>::value, "Dispatched message should inherit from Message class");
        dispatchMessage(std::static_pointer_cast<Message>(msg), name);
    }
    
    // register a listener for a message
    template <typename T, typename R> void Messenger::registerListener(T *receiver, void (T::*method)(std::shared_ptr<R>), std::string message_type) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<Message, R>::value, "Notifier method should take a shared pointer to a message derived from the Message class as argument");
        
        BaseDelegate *delegate = new Delegate<T, R>(receiver, method);
        delegates_[std::type_index(typeid(R))][message_type].push_back(delegate);
    }
    template <typename T, typename R> void Messenger::bindSingle(T *receiver, std::shared_ptr<R> T::*member, std::string message_type) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<Message, R>::value, "Bound variable should be a shared pointer to a message derived from the Message class");
        
        BaseDelegate *delegate = new SingleBindDelegate<T, R>(receiver, member);
        delegates_[std::type_index(typeid(R))][message_type].push_back(delegate);
    }
    
    // FIXME: allow other class containers besides vector
    template <typename T, typename R> void Messenger::bindMulti(T *receiver, std::vector<std::shared_ptr<R>> T::*member, std::string message_type) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<Message, R>::value, "Bound variable should be a shared pointer to a message derived from the Message class");
        
        BaseDelegate *delegate = new VectorBindDelegate<T, R>(receiver, member);
        delegates_[std::type_index(typeid(R))][message_type].push_back(delegate);
    }
    
}

#endif /* ALLPIX_MESSENGER_H */