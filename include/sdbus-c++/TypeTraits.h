/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file TypeTraits.h
 *
 * Created on: Nov 9, 2016
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

#ifndef SDBUS_CXX_TYPETRAITS_H_
#define SDBUS_CXX_TYPETRAITS_H_

#include <sdbus-c++/Error.h>

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#ifdef __has_include
#  if __has_include(<span>)
#    include <span>
#  endif
#endif
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// Forward declarations
namespace sdbus {
    class Variant;
    template <typename... _ValueTypes> class Struct;
    class ObjectPath;
    class Signature;
    class UnixFd;
    template<typename _T1, typename _T2> using DictEntry = std::pair<_T1, _T2>;
    class BusName;
    class InterfaceName;
    class MemberName;
    class MethodCall;
    class MethodReply;
    class Signal;
    class Message;
    class PropertySetCall;
    class PropertyGetReply;
    template <typename... _Results> class Result;
    class Error;
    template <typename _T, typename _Enable = void> struct signature_of;
}

namespace sdbus {

    // Callbacks from sdbus-c++
    using method_callback = std::function<void(MethodCall msg)>;
    using async_reply_handler = std::function<void(MethodReply reply, std::optional<Error> error)>;
    using signal_handler = std::function<void(Signal signal)>;
    using message_handler = std::function<void(Message msg)>;
    using property_set_callback = std::function<void(PropertySetCall msg)>;
    using property_get_callback = std::function<void(PropertyGetReply& reply)>;

    // Type-erased RAII-style handle to callbacks/subscriptions registered to sdbus-c++
    using Slot = std::unique_ptr<void, std::function<void(void*)>>;

    // Tag specifying that an owning handle (so-called slot) of the logical resource shall be provided to the client
    struct return_slot_t { explicit return_slot_t() = default; };
    inline constexpr return_slot_t return_slot{};
    // Tag specifying that the library shall own the slot resulting from the call of the function (so-called floating slot)
    struct floating_slot_t { explicit floating_slot_t() = default; };
    inline constexpr floating_slot_t floating_slot{};
    // Tag denoting the assumption that the caller has already obtained message ownership
    struct adopt_message_t { explicit adopt_message_t() = default; };
    inline constexpr adopt_message_t adopt_message{};
    // Tag denoting the assumption that the caller has already obtained fd ownership
    struct adopt_fd_t { explicit adopt_fd_t() = default; };
    inline constexpr adopt_fd_t adopt_fd{};
    // Tag specifying that the proxy shall not run an event loop thread on its D-Bus connection.
    // Such proxies are typically created to carry out a simple synchronous D-Bus call(s) and then are destroyed.
    struct dont_run_event_loop_thread_t { explicit dont_run_event_loop_thread_t() = default; };
    inline constexpr dont_run_event_loop_thread_t dont_run_event_loop_thread{};
    // Tag denoting an asynchronous call that returns std::future as a handle
    struct with_future_t { explicit with_future_t() = default; };
    inline constexpr with_future_t with_future{};
    // Tag denoting a call where the reply shouldn't be waited for
    struct dont_expect_reply_t { explicit dont_expect_reply_t() = default; };
    inline constexpr dont_expect_reply_t dont_expect_reply{};
    // Tag denoting that the variant shall embed the other variant as its value, instead of creating a copy
    struct embed_variant_t { explicit embed_variant_t() = default; };
    inline constexpr embed_variant_t embed_variant{};

    // Helper for static assert
    template <class... _T> constexpr bool always_false = false;

    // Helper operator+ for concatenation of `std::array`s
    template <typename _T, std::size_t _N1, std::size_t _N2>
    constexpr std::array<_T, _N1 + _N2> operator+(std::array<_T, _N1> lhs, std::array<_T, _N2> rhs);

    // Template specializations for getting D-Bus signatures from C++ types
    template <typename _T>
    constexpr auto signature_of_v = signature_of<_T>::value;

    template <typename _T, typename _Enable>
    struct signature_of
    {
        static constexpr bool is_valid = false;
        static constexpr bool is_trivial_dbus_type = false;

        static constexpr void* value = []
        {
            // See using-sdbus-c++.md, section "Extending sdbus-c++ type system",
            // on how to teach sdbus-c++ about your custom types
            static_assert(always_false<_T>, "Unsupported D-Bus type (specialize `signature_of` for your custom types)");
        };
    };

    template <typename _T>
    struct signature_of<const _T> : signature_of<_T>
    {};

    template <typename _T>
    struct signature_of<volatile _T> : signature_of<_T>
    {};

    template <typename _T>
    struct signature_of<const volatile _T> : signature_of<_T>
    {};

    template <typename _T>
    struct signature_of<_T&> : signature_of<_T>
    {};

    template <>
    struct signature_of<void>
    {
        static constexpr std::array<char, 0> value{};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <>
    struct signature_of<bool>
    {
        static constexpr std::array value{'b'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<uint8_t>
    {
        static constexpr std::array value{'y'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<int16_t>
    {
        static constexpr std::array value{'n'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<uint16_t>
    {
        static constexpr std::array value{'q'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<int32_t>
    {
        static constexpr std::array value{'i'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<uint32_t>
    {
        static constexpr std::array value{'u'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<int64_t>
    {
        static constexpr std::array value{'x'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<uint64_t>
    {
        static constexpr std::array value{'t'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<double>
    {
        static constexpr std::array value{'d'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<std::string>
    {
        static constexpr std::array value{'s'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <>
    struct signature_of<std::string_view> : signature_of<std::string>
    {};

    template <>
    struct signature_of<char*> : signature_of<std::string>
    {};

    template <>
    struct signature_of<const char*> : signature_of<std::string>
    {};

    template <std::size_t _N>
    struct signature_of<char[_N]> : signature_of<std::string>
    {};

    template <std::size_t _N>
    struct signature_of<const char[_N]> : signature_of<std::string>
    {};

    template <>
    struct signature_of<BusName> : signature_of<std::string>
    {};

    template <>
    struct signature_of<InterfaceName> : signature_of<std::string>
    {};

    template <>
    struct signature_of<MemberName> : signature_of<std::string>
    {};

    template <typename... _ValueTypes>
    struct signature_of<Struct<_ValueTypes...>>
    {
        static constexpr std::array contents = (signature_of_v<_ValueTypes> + ...);
        static constexpr std::array value = std::array{'('} + contents + std::array{')'};
        static constexpr char type_value{'r'}; /* Not actually used in signatures on D-Bus, see specs */
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <>
    struct signature_of<Variant>
    {
        static constexpr std::array value{'v'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <typename... Elements>
    struct signature_of<std::variant<Elements...>> : signature_of<Variant>
    {};

    template <>
    struct signature_of<ObjectPath>
    {
        static constexpr std::array value{'o'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <>
    struct signature_of<Signature>
    {
        static constexpr std::array value{'g'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <>
    struct signature_of<UnixFd>
    {
        static constexpr std::array value{'h'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <typename _T1, typename _T2>
    struct signature_of<DictEntry<_T1, _T2>>
    {
        static constexpr std::array value = std::array{'{'} + signature_of_v<std::tuple<_T1, _T2>> + std::array{'}'};
        static constexpr char type_value{'e'}; /* Not actually used in signatures on D-Bus, see specs */
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <typename _Element, typename _Allocator>
    struct signature_of<std::vector<_Element, _Allocator>>
    {
        static constexpr std::array value = std::array{'a'} + signature_of_v<_Element>;
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <typename _Element, std::size_t _Size>
    struct signature_of<std::array<_Element, _Size>> : signature_of<std::vector<_Element>>
    {
    };

#ifdef __cpp_lib_span
    template <typename _Element, std::size_t _Extent>
    struct signature_of<std::span<_Element, _Extent>> : signature_of<std::vector<_Element>>
    {
    };
#endif

    template <typename _Enum> // is_const_v and is_volatile_v to avoid ambiguity conflicts with const and volatile specializations of signature_of
    struct signature_of<_Enum, typename std::enable_if_t<std::is_enum_v<_Enum> && !std::is_const_v<_Enum> && !std::is_volatile_v<_Enum>>>
        : signature_of<std::underlying_type_t<_Enum>>
    {};

    template <typename _Key, typename _Value, typename _Compare, typename _Allocator>
    struct signature_of<std::map<_Key, _Value, _Compare, _Allocator>>
    {
        static constexpr std::array value = std::array{'a'} + signature_of_v<DictEntry<_Key, _Value>>;
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <typename _Key, typename _Value, typename _Hash, typename _KeyEqual, typename _Allocator>
    struct signature_of<std::unordered_map<_Key, _Value, _Hash, _KeyEqual, _Allocator>>
        : signature_of<std::map<_Key, _Value>>
    {
    };

    template <typename... _Types>
    struct signature_of<std::tuple<_Types...>> // A simple concatenation of signatures of _Types
    {
        static constexpr std::array value = (std::array<char, 0>{} + ... + signature_of_v<_Types>);
        static constexpr bool is_valid = false;
        static constexpr bool is_trivial_dbus_type = false;
    };

    // To simplify conversions of arrays to C strings
    template <typename _T, std::size_t _N>
    constexpr auto as_null_terminated(std::array<_T, _N> arr)
    {
        return arr + std::array<_T, 1>{0};
    }

    // Function traits implementation inspired by (c) kennytm,
    // https://github.com/kennytm/utils/blob/master/traits.hpp
    template <typename _Type>
    struct function_traits : function_traits<decltype(&_Type::operator())>
    {};

    template <typename _Type>
    struct function_traits<const _Type> : function_traits<_Type>
    {};

    template <typename _Type>
    struct function_traits<_Type&> : function_traits<_Type>
    {};

    template <typename _ReturnType, typename... _Args>
    struct function_traits_base
    {
        typedef _ReturnType result_type;
        typedef std::tuple<_Args...> arguments_type;
        typedef std::tuple<std::decay_t<_Args>...> decayed_arguments_type;

        typedef _ReturnType function_type(_Args...);

        static constexpr std::size_t arity = sizeof...(_Args);

//        template <size_t _Idx, typename _Enabled = void>
//        struct arg;
//
//        template <size_t _Idx>
//        struct arg<_Idx, std::enable_if_t<(_Idx < arity)>>
//        {
//            typedef std::tuple_element_t<_Idx, arguments_type> type;
//        };
//
//        template <size_t _Idx>
//        struct arg<_Idx, std::enable_if_t<!(_Idx < arity)>>
//        {
//            typedef void type;
//        };

        template <size_t _Idx>
        struct arg
        {
            typedef std::tuple_element_t<_Idx, std::tuple<_Args...>> type;
        };

        template <size_t _Idx>
        using arg_t = typename arg<_Idx>::type;
    };

    template <typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(_Args...)> : function_traits_base<_ReturnType, _Args...>
    {
        static constexpr bool is_async = false;
        static constexpr bool has_error_param = false;
    };

    template <typename... _Args>
    struct function_traits<void(std::optional<Error>, _Args...)> : function_traits_base<void, _Args...>
    {
        static constexpr bool has_error_param = true;
    };

    template <typename... _Args, typename... _Results>
    struct function_traits<void(Result<_Results...>, _Args...)> : function_traits_base<std::tuple<_Results...>, _Args...>
    {
        static constexpr bool is_async = true;
        using async_result_t = Result<_Results...>;
    };

    template <typename... _Args, typename... _Results>
    struct function_traits<void(Result<_Results...>&&, _Args...)> : function_traits_base<std::tuple<_Results...>, _Args...>
    {
        static constexpr bool is_async = true;
        using async_result_t = Result<_Results...>;
    };

    template <typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(*)(_Args...)> : function_traits<_ReturnType(_Args...)>
    {};

    template <typename _ClassType, typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(_ClassType::*)(_Args...)> : function_traits<_ReturnType(_Args...)>
    {
        typedef _ClassType& owner_type;
    };

    template <typename _ClassType, typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(_ClassType::*)(_Args...) const> : function_traits<_ReturnType(_Args...)>
    {
        typedef const _ClassType& owner_type;
    };

    template <typename _ClassType, typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(_ClassType::*)(_Args...) volatile> : function_traits<_ReturnType(_Args...)>
    {
        typedef volatile _ClassType& owner_type;
    };

    template <typename _ClassType, typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(_ClassType::*)(_Args...) const volatile> : function_traits<_ReturnType(_Args...)>
    {
        typedef const volatile _ClassType& owner_type;
    };

    template <typename FunctionType>
    struct function_traits<std::function<FunctionType>> : function_traits<FunctionType>
    {};

    template <class _Function>
    constexpr auto is_async_method_v = function_traits<_Function>::is_async;

    template <class _Function>
    constexpr auto has_error_param_v = function_traits<_Function>::has_error_param;

    template <typename _FunctionType>
    using function_arguments_t = typename function_traits<_FunctionType>::arguments_type;

    template <typename _FunctionType, size_t _Idx>
    using function_argument_t = typename function_traits<_FunctionType>::template arg_t<_Idx>;

    template <typename _FunctionType>
    constexpr auto function_argument_count_v = function_traits<_FunctionType>::arity;

    template <typename _FunctionType>
    using function_result_t = typename function_traits<_FunctionType>::result_type;

    template <typename _Function>
    struct tuple_of_function_input_arg_types
    {
        typedef typename function_traits<_Function>::decayed_arguments_type type;
    };

    template <typename _Function>
    using tuple_of_function_input_arg_types_t = typename tuple_of_function_input_arg_types<_Function>::type;

    template <typename _Function>
    struct tuple_of_function_output_arg_types
    {
        typedef typename function_traits<_Function>::result_type type;
    };

    template <typename _Function>
    using tuple_of_function_output_arg_types_t = typename tuple_of_function_output_arg_types<_Function>::type;

    template <typename _Function>
    struct signature_of_function_input_arguments : signature_of<tuple_of_function_input_arg_types_t<_Function>>
    {
        static std::string value_as_string()
        {
            constexpr auto signature = as_null_terminated(signature_of_v<tuple_of_function_input_arg_types_t<_Function>>);
            return signature.data();
        }
    };

    template <typename _Function>
    inline auto signature_of_function_input_arguments_v = signature_of_function_input_arguments<_Function>::value_as_string();

    template <typename _Function>
    struct signature_of_function_output_arguments : signature_of<tuple_of_function_output_arg_types_t<_Function>>
    {
        static std::string value_as_string()
        {
            constexpr auto signature = as_null_terminated(signature_of_v<tuple_of_function_output_arg_types_t<_Function>>);
            return signature.data();
        }
    };

    template <typename _Function>
    inline auto signature_of_function_output_arguments_v = signature_of_function_output_arguments<_Function>::value_as_string();

    // std::future stuff for return values of async calls
    template <typename... _Args> struct future_return
    {
        typedef std::tuple<_Args...> type;
    };

    template <> struct future_return<>
    {
        typedef void type;
    };

    template <typename _Type> struct future_return<_Type>
    {
        typedef _Type type;
    };

    template <typename... _Args>
    using future_return_t = typename future_return<_Args...>::type;

    // Credit: Piotr Skotnicki (https://stackoverflow.com/a/57639506)
    template <typename, typename>
    constexpr bool is_one_of_variants_types = false;

    template <typename... _VariantTypes, typename _QueriedType>
    constexpr bool is_one_of_variants_types<std::variant<_VariantTypes...>, _QueriedType>
        = (std::is_same_v<_QueriedType, _VariantTypes> || ...);

    // Wrapper (tag) denoting we want to serialize user-defined struct
    // into a D-Bus message as a dictionary of strings to variants.
    template <typename _Struct>
    struct as_dictionary
    {
        explicit as_dictionary(const _Struct& s) : m_struct(s) {}
        const _Struct& m_struct;
    };

    template <typename _Type>
    const _Type& as_dictionary_if_struct(const _Type& object)
    {
        return object; // identity in case _Type is not struct (user-defined structs shall provide an overload)
    }

    // By default, the dict-as-struct deserialization strategy is strict.
    // Strict means that every key of the deserialized dictionary must have its counterpart member in the struct, otherwise an exception is thrown.
    // Relaxed means that a key that does not have a matching struct member is silently ignored.
    // The behavior can be overridden for user-defined struct by specializing this variable template.
    template <typename _Struct>
    constexpr auto strict_dict_as_struct_deserialization_v = true;

    // By default, the struct-as-dict serialization strategy is single-level only (as opposed to nested).
    // Single-level means that only the specific struct is serialized as a dictionary, serializing members that are structs always as structs.
    // Nested means that the struct *and* its members that are structs are all serialized as a dictionary. If nested strategy is also
    // defined for the nested struct, then the same behavior applies for that struct, recursively.
    // The behavior can be overridden for user-defined struct by specializing this variable template.
    template <typename _Struct>
    constexpr auto nested_struct_as_dict_serialization_v = false;

    namespace detail
    {
        template <class _Function, class _Tuple, typename... _Args, std::size_t... _I>
        constexpr decltype(auto) apply_impl( _Function&& f
                                           , Result<_Args...>&& r
                                           , _Tuple&& t
                                           , std::index_sequence<_I...> )
        {
            return std::forward<_Function>(f)(std::move(r), std::get<_I>(std::forward<_Tuple>(t))...);
        }

        template <class _Function, class _Tuple, std::size_t... _I>
        decltype(auto) apply_impl( _Function&& f
                                 , std::optional<Error> e
                                 , _Tuple&& t
                                 , std::index_sequence<_I...> )
        {
            return std::forward<_Function>(f)(std::move(e), std::get<_I>(std::forward<_Tuple>(t))...);
        }

        // For non-void returning functions, apply_impl simply returns function return value (a tuple of values).
        // For void-returning functions, apply_impl returns an empty tuple.
        template <class _Function, class _Tuple, std::size_t... _I>
        constexpr decltype(auto) apply_impl( _Function&& f
                                           , _Tuple&& t
                                           , std::index_sequence<_I...> )
        {
            if constexpr (!std::is_void_v<function_result_t<_Function>>)
                return std::forward<_Function>(f)(std::get<_I>(std::forward<_Tuple>(t))...);
            else
                return std::forward<_Function>(f)(std::get<_I>(std::forward<_Tuple>(t))...), std::tuple<>{};
        }
    }

    // Convert tuple `t' of values into a list of arguments
    // and invoke function `f' with those arguments.
    template <class _Function, class _Tuple>
    constexpr decltype(auto) apply(_Function&& f, _Tuple&& t)
    {
        return detail::apply_impl( std::forward<_Function>(f)
                                 , std::forward<_Tuple>(t)
                                 , std::make_index_sequence<std::tuple_size<std::decay_t<_Tuple>>::value>{} );
    }

    // Convert tuple `t' of values into a list of arguments
    // and invoke function `f' with those arguments.
    template <class _Function, class _Tuple, typename... _Args>
    constexpr decltype(auto) apply(_Function&& f, Result<_Args...>&& r, _Tuple&& t)
    {
        return detail::apply_impl( std::forward<_Function>(f)
                                 , std::move(r)
                                 , std::forward<_Tuple>(t)
                                 , std::make_index_sequence<std::tuple_size<std::decay_t<_Tuple>>::value>{} );
    }

    // Convert tuple `t' of values into a list of arguments
    // and invoke function `f' with those arguments.
    template <class _Function, class _Tuple>
    decltype(auto) apply(_Function&& f, std::optional<Error> e, _Tuple&& t)
    {
        return detail::apply_impl( std::forward<_Function>(f)
                                 , std::move(e)
                                 , std::forward<_Tuple>(t)
                                 , std::make_index_sequence<std::tuple_size<std::decay_t<_Tuple>>::value>{} );
    }

    // Convenient concatenation of arrays
    template <typename _T, std::size_t _N1, std::size_t _N2>
    constexpr std::array<_T, _N1 + _N2> operator+(std::array<_T, _N1> lhs, std::array<_T, _N2> rhs)
    {
        std::array<_T, _N1 + _N2> result{};
        std::size_t index = 0;

        for (auto& el : lhs) {
            result[index] = std::move(el);
            ++index;
        }
        for (auto& el : rhs) {
            result[index] = std::move(el);
            ++index;
        }

        return result;
    }

}

#endif /* SDBUS_CXX_TYPETRAITS_H_ */
