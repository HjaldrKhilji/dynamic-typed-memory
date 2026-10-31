#include <meta>
#include <format>
#include <type_traits>
#include <variant>
#include <functional>
#include <array>
#include<utility>
//runtime_indexed_tuples( https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4198r0.pdf )will be used here to provide fixed size heterogenous lists(for faster indexing), though as you will see, some aspects have been modified:
namespace storage{
    template<template<std::size_t N, typename...> class F, std::size_t N,
    typename ... Args>
    using Delay = F<N, Args...>;
    template< std::size_t N, typename Head, typename ...Types>
    struct get_variant_of_unique_ts :
    std::conditional_t<
    (std::is_same_v<Head, Types> || ...),
    Delay<get_variant_of_unique_ts, N - 1, Types... >,
    Delay<get_variant_of_unique_ts, N - 1, Head, Types... >
    >
    {
        using intermediate =
        std::conditional_t< (std::is_same_v<Head, Types> || ...),
        Delay<get_variant_of_unique_ts, N - 1, Types... >,
        Delay<get_variant_of_unique_ts, N - 1, Head, Types... >
        >;
        using Variant_t = typename intermediate::Variant_t;
        using List_t    = typename intermediate::List_t;
        using intermediate::return_type_list;
    };
    template<typename Head, typename... Types>
    struct get_variant_of_unique_ts<0, Head, Types...>
    {
        using Variant_t = std::variant<Head*, Types*...>;
        using List_t=std::array<std::meta::info, 1+sizeof...(Types)>;
        static constexpr List_t return_type_list(){
            return List_t{^^Head, ^^Types...};
        }
    };
    template<typename ... Types>
    struct runtime_indexed_tuples {
        std::tuple<Types...> data;
        using type_folder=get_variant_of_unique_ts<sizeof...(Types)
        - 1, Types...>;
        type_folder::List_t type_reflections;//dosent include duplicates
        using value_type = type_folder::Variant_t;
        using Book_keeping_t = std::array<value_type, (sizeof...(Types))>;
        Book_keeping_t book_keeper;
        template<typename... Types_>
        constexpr runtime_indexed_tuples(Types_&&... b) :
        data{ std::tuple(std::forward<Types_>(b)...) },
        book_keeper{
            std::apply([](auto&... args) {
                return Book_keeping_t{args...};
            }, data)
        },
        type_reflections{
            type_folder::return_type_list()
        }
        {}
        constexpr value_type operator[](size_t index) {
            return book_keeper[index];
        }
    };
};

namespace runtime{
    template<typename... Ts>
    class Dynamic_unit{

        static  inline std::array<std::string, sizeof...(Ts)+(sizeof...(Ts)*sizeof...(Ts))> tag_name_buffer;
        static  inline std::size_t index=0;
        constexpr std::string& get_buffer_slot(){
            if(index==tag_name_buffer.lenght()){
                index=0;
            }
            index++;
            return tag_name_buffer[index-1];
        }
        using List_t=std::array<std::meta::info, sizeof...(Ts)>;
        void *ptr;
        enum class Single_tag_type
        {
            empty
        };
        enum class Dual_Tag_type
        {
            empty
        };
        std::array<Dual_Tag_type,sizeof...(Ts)> all_dual_tag_enums;//for the template for loop in do_with_value
        constexpr static List_t all_type_reflections{^^Ts...}; //include duplicates
        Single_tag_type tag;
        consteval static std::string construct_name(std::string type_name_1, std::string type_name_2){
            return type_name_1+type_name_2;
        }
        consteval static List_t construct_dual_type_list(auto dummy){
            List_t members;
            auto types= std::to_array({^^Ts...});
            auto name_of_dummy =std::meta::identifier_of(dummy);
            for(auto Type: {^^Ts...}) //t_it=test_iterator
            {
                auto name_of_member= std::meta::identifier_of(Type);
                for(auto N_type: {^^Ts...}){
                    get_buffer_slot()=construct_name(name_of_member, std::meta::identifier_of(N_type));
                    const_cast<std::string_view>(name_of_dummy)= get_buffer_slot();
                    members.push_back(dummy);
                }
            }
            return members;
        }
        consteval static List_t construct_single_type_list(auto dummy){
            List_t members;
            for (auto Type: {^^Ts...})
            {
                auto name_of_member= std::meta::identifier_of(Type);
                auto name_of_dummy =std::meta::identifier_of(dummy);
                get_buffer_slot()=name_of_member;
                const_cast<std::string_view>(name_of_dummy)=get_buffer_slot();
                members.push_back(dummy);
            }
            return members;
        }
        consteval static void construct_dual_tag(auto dummy){
            std::meta::define_aggregate(^^Dual_Tag_type, construct_dual_type_list(dummy));
        }
        consteval static void construct_single_tag(auto dummy){
            std::meta::define_aggregate(^^Single_tag_type, construct_single_type_list(dummy));

        }
        consteval void push_all_enumerators(){
            constexpr static auto enumerators = std::define_static_array(enumerators_of(^^Dual_Tag_type));
            template for (constexpr auto enumerator : enumerators)
            {
                all_dual_tag_enums.push(^^enumerator);
            }
        }
        consteval
        {
            construct_dual_tag(^^Dual_Tag_type::empty);
            construct_single_tag(^^Single_tag_type::empty);
            push_all_enumerators();
        }
        void do_with_value(Dynamic_unit lhs, std::size_t op_index){
            template for(auto enumerator: all_dual_tag_enums){
                if(enumerator==((tag>>1)^lhs.tag) ){ //cryptographers say that all they do is to use XOR, hence I used it to flatten stuff up for the cache and branch predictor's sake.
                    static_cast<[:all_type_reflections[tag]:]>(ptr); //todo on how to use op_index
                }
            }
        }
    };

    template<typename... Ts>
    struct Runtime_storage_t{
        std::vector<Dynamic_unit<Ts...>> all_storage;
    };

};


int main()
{

    return 0;
}
