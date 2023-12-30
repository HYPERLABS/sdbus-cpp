/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file ConvenienceApiClasses.inl
 *
 * Created on: Dec 19, 2016
 * Project: sdbus-c++
 * Description: High-level D-Bus IPC C++ library based on sd-bus
 *
 * This file is part of sdbus-c++.
 *
 * sdbus-c++ is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * sdbus-c++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with sdbus-c++. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SDBUS_CPP_CONVENIENCEAPICLASSES_INL_
#define SDBUS_CPP_CONVENIENCEAPICLASSES_INL_

#include <sdbus-c++/IObject.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/MethodResult.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/TypeTraits.h>
#include <sdbus-c++/Error.h>
#include <type_traits>
#include <string>
#include <tuple>
#include <exception>
#include <cassert>

namespace sdbus {

    /*** ------------- ***/
    /***  VTableAdder  ***/
    /*** ------------- ***/

    inline VTableAdder::VTableAdder(IObject& object, std::vector<VTableItem> vtable)
        : object_(object)
        , vtable_(std::move(vtable))
    {
    }

    inline void VTableAdder::forInterface(std::string interfaceName)
    {
        object_.addVTable(std::move(interfaceName), std::move(vtable_));
    }

    inline Slot VTableAdder::forInterface(std::string interfaceName, request_slot_t)
    {
        return object_.addVTable(std::move(interfaceName), std::move(vtable_), request_slot);
    }

    /*** ------------- ***/
    /*** SignalEmitter ***/
    /*** ------------- ***/

    inline SignalEmitter::SignalEmitter(IObject& object, const std::string& signalName)
        : object_(object)
        , signalName_(signalName)
        , exceptions_(std::uncaught_exceptions())
    {
    }

    inline SignalEmitter::~SignalEmitter() noexcept(false) // since C++11, destructors must
    {                                                      // explicitly be allowed to throw
        // Don't emit the signal if SignalEmitter threw an exception in one of its methods
        if (std::uncaught_exceptions() != exceptions_)
            return;

        // emitSignal() can throw. But as the SignalEmitter shall always be used as an unnamed,
        // temporary object, i.e. not as a stack-allocated object, the double-exception situation
        // shall never happen. I.e. it should not happen that this destructor is directly called
        // in the stack-unwinding process of another flying exception (which would lead to immediate
        // termination). It can be called indirectly in the destructor of another object, but that's
        // fine and safe provided that the caller catches exceptions thrown from here.
        // Therefore, we can allow emitSignal() to throw even if we are in the destructor.
        // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
        // to the exception thrown from here if the caller is a destructor itself.
        object_.emitSignal(signal_);
    }

    inline SignalEmitter& SignalEmitter::onInterface(const std::string& interfaceName)
    {
        signal_ = object_.createSignal(interfaceName, signalName_);

        return *this;
    }

    template <typename... _Args>
    inline void SignalEmitter::withArguments(_Args&&... args)
    {
        assert(signal_.isValid()); // onInterface() must be placed/called prior to withArguments()

        detail::serialize_pack(signal_, std::forward<_Args>(args)...);
    }

    /*** ------------- ***/
    /*** MethodInvoker ***/
    /*** ------------- ***/

    inline MethodInvoker::MethodInvoker(IProxy& proxy, const std::string& methodName)
        : proxy_(proxy)
        , methodName_(methodName)
        , exceptions_(std::uncaught_exceptions())
    {
    }

    inline MethodInvoker::~MethodInvoker() noexcept(false) // since C++11, destructors must
    {                                                      // explicitly be allowed to throw
        // Don't call the method if it has been called already or if MethodInvoker
        // threw an exception in one of its methods
        if (methodCalled_ || std::uncaught_exceptions() != exceptions_)
            return;

        // callMethod() can throw. But as the MethodInvoker shall always be used as an unnamed,
        // temporary object, i.e. not as a stack-allocated object, the double-exception situation
        // shall never happen. I.e. it should not happen that this destructor is directly called
        // in the stack-unwinding process of another flying exception (which would lead to immediate
        // termination). It can be called indirectly in the destructor of another object, but that's
        // fine and safe provided that the caller catches exceptions thrown from here.
        // Therefore, we can allow callMethod() to throw even if we are in the destructor.
        // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
        // to the exception thrown from here if the caller is a destructor itself.
        proxy_.callMethod(method_, timeout_);
    }

    inline MethodInvoker& MethodInvoker::onInterface(const std::string& interfaceName)
    {
        method_ = proxy_.createMethodCall(interfaceName, methodName_);

        return *this;
    }

    inline MethodInvoker& MethodInvoker::withTimeout(uint64_t usec)
    {
        timeout_ = usec;

        return *this;
    }

    template <typename _Rep, typename _Period>
    inline MethodInvoker& MethodInvoker::withTimeout(const std::chrono::duration<_Rep, _Period>& timeout)
    {
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
        return withTimeout(microsecs.count());
    }

    template <typename... _Args>
    inline MethodInvoker& MethodInvoker::withArguments(_Args&&... args)
    {
        assert(method_.isValid()); // onInterface() must be placed/called prior to this function

        detail::serialize_pack(method_, std::forward<_Args>(args)...);

        return *this;
    }

    template <typename... _Args>
    inline void MethodInvoker::storeResultsTo(_Args&... args)
    {
        assert(method_.isValid()); // onInterface() must be placed/called prior to this function

        auto reply = proxy_.callMethod(method_, timeout_);
        methodCalled_ = true;

        detail::deserialize_pack(reply, args...);
    }

    inline void MethodInvoker::dontExpectReply()
    {
        assert(method_.isValid()); // onInterface() must be placed/called prior to this function

        method_.dontExpectReply();
    }

    /*** ------------------ ***/
    /*** AsyncMethodInvoker ***/
    /*** ------------------ ***/

    inline AsyncMethodInvoker::AsyncMethodInvoker(IProxy& proxy, const std::string& methodName)
        : proxy_(proxy)
        , methodName_(methodName)
    {
    }

    inline AsyncMethodInvoker& AsyncMethodInvoker::onInterface(const std::string& interfaceName)
    {
        method_ = proxy_.createMethodCall(interfaceName, methodName_);

        return *this;
    }

    inline AsyncMethodInvoker& AsyncMethodInvoker::withTimeout(uint64_t usec)
    {
        timeout_ = usec;

        return *this;
    }

    template <typename _Rep, typename _Period>
    inline AsyncMethodInvoker& AsyncMethodInvoker::withTimeout(const std::chrono::duration<_Rep, _Period>& timeout)
    {
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
        return withTimeout(microsecs.count());
    }

    template <typename... _Args>
    inline AsyncMethodInvoker& AsyncMethodInvoker::withArguments(_Args&&... args)
    {
        assert(method_.isValid()); // onInterface() must be placed/called prior to this function

        detail::serialize_pack(method_, std::forward<_Args>(args)...);

        return *this;
    }

    template <typename _Function>
    PendingAsyncCall AsyncMethodInvoker::uponReplyInvoke(_Function&& callback)
    {
        assert(method_.isValid()); // onInterface() must be placed/called prior to this function

        auto asyncReplyHandler = [callback = std::forward<_Function>(callback)](MethodReply reply, const Error* error)
        {
            // Create a tuple of callback input arguments' types, which will be used
            // as a storage for the argument values deserialized from the message.
            tuple_of_function_input_arg_types_t<_Function> args;

            // Deserialize input arguments from the message into the tuple (if no error occurred).
            if (error == nullptr)
            {
                try
                {
                    reply >> args;
                }
                catch (const Error& e)
                {
                    // Pass message deserialization exceptions to the client via callback error parameter,
                    // instead of propagating them up the message loop call stack.
                    sdbus::apply(callback, &e, args);
                    return;
                }
            }

            // Invoke callback with input arguments from the tuple.
            sdbus::apply(callback, error, args);
        };

        return proxy_.callMethod(method_, std::move(asyncReplyHandler), timeout_);
    }

    template <typename... _Args>
    std::future<future_return_t<_Args...>> AsyncMethodInvoker::getResultAsFuture()
    {
        auto promise = std::make_shared<std::promise<future_return_t<_Args...>>>();
        auto future = promise->get_future();

        uponReplyInvoke([promise = std::move(promise)](const Error* error, _Args... args)
        {
            if (error == nullptr)
                if constexpr (!std::is_void_v<future_return_t<_Args...>>)
                    promise->set_value({std::move(args)...});
                else
                    promise->set_value();
            else
                promise->set_exception(std::make_exception_ptr(*error));
        });

        // Will be std::future<void> for no D-Bus method return value
        //      or std::future<T> for single D-Bus method return value
        //      or std::future<std::tuple<...>> for multiple method return values
        return future;
    }

    /*** ---------------- ***/
    /*** SignalSubscriber ***/
    /*** ---------------- ***/

    inline SignalSubscriber::SignalSubscriber(IProxy& proxy, const std::string& signalName)
        : proxy_(proxy)
        , signalName_(signalName)
    {
    }

    inline SignalSubscriber& SignalSubscriber::onInterface(std::string interfaceName)
    {
        interfaceName_ = std::move(interfaceName);

        return *this;
    }

    template <typename _Function>
    inline void SignalSubscriber::call(_Function&& callback)
    {
        assert(!interfaceName_.empty()); // onInterface() must be placed/called prior to this function

        proxy_.registerSignalHandler( interfaceName_
                                    , signalName_
                                    , [callback = std::forward<_Function>(callback)](Signal signal)
        {
            // Create a tuple of callback input arguments' types, which will be used
            // as a storage for the argument values deserialized from the signal message.
            tuple_of_function_input_arg_types_t<_Function> signalArgs;

            // The signal handler can take pure signal parameters only, or an additional `const Error*` as its first
            // parameter. In the former case, if the deserialization fails (e.g. due to signature mismatch),
            // the failure is ignored (and signal simply dropped). In the latter case, the deserialization failure
            // will be communicated as a non-zero Error pointer to the client's signal handler.
            if constexpr (has_error_param_v<_Function>)
            {
                // Deserialize input arguments from the signal message into the tuple
                try
                {
                    signal >> signalArgs;
                }
                catch (const sdbus::Error& e)
                {
                    // Pass message deserialization exceptions to the client via callback error parameter,
                    // instead of propagating them up the message loop call stack.
                    sdbus::apply(callback, &e, signalArgs);
                    return;
                }

                // Invoke callback with no error and input arguments from the tuple.
                sdbus::apply(callback, nullptr, signalArgs);
            }
            else
            {
                // Deserialize input arguments from the signal message into the tuple
                signal >> signalArgs;

                // Invoke callback with input arguments from the tuple.
                sdbus::apply(callback, signalArgs);
            }
        });
    }

    /*** ------------------ ***/
    /*** SignalUnsubscriber ***/
    /*** ------------------ ***/

    inline SignalUnsubscriber::SignalUnsubscriber(IProxy& proxy, const std::string& signalName)
        : proxy_(proxy)
        , signalName_(signalName)
    {
    }

    inline void SignalUnsubscriber::onInterface(const std::string& interfaceName)
    {
        proxy_.unregisterSignalHandler(interfaceName, signalName_);
    }

    /*** -------------- ***/
    /*** PropertyGetter ***/
    /*** -------------- ***/

    inline PropertyGetter::PropertyGetter(IProxy& proxy, const std::string& propertyName)
        : proxy_(proxy)
        , propertyName_(propertyName)
    {
    }

    inline Variant PropertyGetter::onInterface(const std::string& interfaceName)
    {
        Variant var;
        proxy_.callMethod("Get")
              .onInterface("org.freedesktop.DBus.Properties")
              .withArguments(interfaceName, propertyName_)
              .storeResultsTo(var);
        return var;
    }

    /*** ------------------- ***/
    /*** AsyncPropertyGetter ***/
    /*** ------------------- ***/

    inline AsyncPropertyGetter::AsyncPropertyGetter(IProxy& proxy, const std::string& propertyName)
            : proxy_(proxy)
            , propertyName_(propertyName)
    {
    }

    inline AsyncPropertyGetter& AsyncPropertyGetter::onInterface(const std::string& interfaceName)
    {
        interfaceName_ = &interfaceName;

        return *this;
    }

    template <typename _Function>
    PendingAsyncCall AsyncPropertyGetter::uponReplyInvoke(_Function&& callback)
    {
        static_assert(std::is_invocable_r_v<void, _Function, const Error*, Variant>, "Property get callback function must accept Error* and property value as Variant");

        assert(interfaceName_ != nullptr); // onInterface() must be placed/called prior to this function

        return proxy_.callMethodAsync("Get")
                     .onInterface("org.freedesktop.DBus.Properties")
                     .withArguments(*interfaceName_, propertyName_)
                     .uponReplyInvoke(std::forward<_Function>(callback));
    }

    inline std::future<Variant> AsyncPropertyGetter::getResultAsFuture()
    {
        assert(interfaceName_ != nullptr); // onInterface() must be placed/called prior to this function

        return proxy_.callMethodAsync("Get")
                     .onInterface("org.freedesktop.DBus.Properties")
                     .withArguments(*interfaceName_, propertyName_)
                     .getResultAsFuture<Variant>();
    }

    /*** -------------- ***/
    /*** PropertySetter ***/
    /*** -------------- ***/

    inline PropertySetter::PropertySetter(IProxy& proxy, const std::string& propertyName)
        : proxy_(proxy)
        , propertyName_(propertyName)
    {
    }

    inline PropertySetter& PropertySetter::onInterface(const std::string& interfaceName)
    {
        interfaceName_ = &interfaceName;

        return *this;
    }

    template <typename _Value>
    inline void PropertySetter::toValue(const _Value& value)
    {
        PropertySetter::toValue(Variant{value});
    }

    template <typename _Value>
    inline void PropertySetter::toValue(const _Value& value, dont_expect_reply_t)
    {
        PropertySetter::toValue(Variant{value}, dont_expect_reply);
    }

    inline void PropertySetter::toValue(const Variant& value)
    {
        assert(interfaceName_ != nullptr); // onInterface() must be placed/called prior to this function

        proxy_.callMethod("Set")
              .onInterface("org.freedesktop.DBus.Properties")
              .withArguments(*interfaceName_, propertyName_, value);
    }

    inline void PropertySetter::toValue(const Variant& value, dont_expect_reply_t)
    {
        assert(interfaceName_ != nullptr); // onInterface() must be placed/called prior to this function

        proxy_.callMethod("Set")
                .onInterface("org.freedesktop.DBus.Properties")
                .withArguments(*interfaceName_, propertyName_, value)
                .dontExpectReply();
    }

    /*** ------------------- ***/
    /*** AsyncPropertySetter ***/
    /*** ------------------- ***/

    inline AsyncPropertySetter::AsyncPropertySetter(IProxy& proxy, const std::string& propertyName)
            : proxy_(proxy)
            , propertyName_(propertyName)
    {
    }

    inline AsyncPropertySetter& AsyncPropertySetter::onInterface(const std::string& interfaceName)
    {
        interfaceName_ = &interfaceName;

        return *this;
    }

    template <typename _Value>
    inline AsyncPropertySetter& AsyncPropertySetter::toValue(_Value&& value)
    {
        return AsyncPropertySetter::toValue(Variant{std::forward<_Value>(value)});
    }

    inline AsyncPropertySetter& AsyncPropertySetter::toValue(Variant value)
    {
        value_ = std::move(value);

        return *this;
    }

    template <typename _Function>
    PendingAsyncCall AsyncPropertySetter::uponReplyInvoke(_Function&& callback)
    {
        static_assert(std::is_invocable_r_v<void, _Function, const Error*>, "Property set callback function must accept Error* only");

        assert(interfaceName_ != nullptr); // onInterface() must be placed/called prior to this function

        return proxy_.callMethodAsync("Set")
                     .onInterface("org.freedesktop.DBus.Properties")
                     .withArguments(*interfaceName_, propertyName_, std::move(value_))
                     .uponReplyInvoke(std::forward<_Function>(callback));
    }

    inline std::future<void> AsyncPropertySetter::getResultAsFuture()
    {
        assert(interfaceName_ != nullptr); // onInterface() must be placed/called prior to this function

        return proxy_.callMethodAsync("Set")
                     .onInterface("org.freedesktop.DBus.Properties")
                     .withArguments(*interfaceName_, propertyName_, std::move(value_))
                     .getResultAsFuture<>();
    }

    /*** ------------------- ***/
    /*** AllPropertiesGetter ***/
    /*** ------------------- ***/

    inline AllPropertiesGetter::AllPropertiesGetter(IProxy& proxy)
            : proxy_(proxy)
    {
    }

    inline std::map<std::string, Variant> AllPropertiesGetter::onInterface(const std::string& interfaceName)
    {
        std::map<std::string, Variant> props;
        proxy_.callMethod("GetAll")
                .onInterface("org.freedesktop.DBus.Properties")
                .withArguments(interfaceName)
                .storeResultsTo(props);
        return props;
    }

    /*** ------------------------ ***/
    /*** AsyncAllPropertiesGetter ***/
    /*** ------------------------ ***/

    inline AsyncAllPropertiesGetter::AsyncAllPropertiesGetter(IProxy& proxy)
            : proxy_(proxy)
    {
    }

    inline AsyncAllPropertiesGetter& AsyncAllPropertiesGetter::onInterface(const std::string& interfaceName)
    {
        interfaceName_ = &interfaceName;

        return *this;
    }

    template <typename _Function>
    PendingAsyncCall AsyncAllPropertiesGetter::uponReplyInvoke(_Function&& callback)
    {
        static_assert( std::is_invocable_r_v<void, _Function, const Error*, std::map<std::string, Variant>>
                     , "All properties get callback function must accept Error* and a map of property names to their values" );

        assert(interfaceName_ != nullptr); // onInterface() must be placed/called prior to this function

        return proxy_.callMethodAsync("GetAll")
                     .onInterface("org.freedesktop.DBus.Properties")
                     .withArguments(*interfaceName_)
                     .uponReplyInvoke(std::forward<_Function>(callback));
    }

    inline std::future<std::map<std::string, Variant>> AsyncAllPropertiesGetter::getResultAsFuture()
    {
        assert(interfaceName_ != nullptr); // onInterface() must be placed/called prior to this function

        return proxy_.callMethodAsync("GetAll")
                     .onInterface("org.freedesktop.DBus.Properties")
                     .withArguments(*interfaceName_)
                     .getResultAsFuture<std::map<std::string, Variant>>();
    }

} // namespace sdbus

#endif /* SDBUS_CPP_CONVENIENCEAPICLASSES_INL_ */
