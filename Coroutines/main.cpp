/**============================================================================
Name        : main.cpp
Created on  : 24.12.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Asio_main.cppTcpServer
============================================================================**/

#include <iostream>
#include <vector>

#include <boost/coroutine2/coroutine.hpp>

namespace demo_1
{
    void producer(boost::coroutines2::coroutine<int>::push_type& yield)
    {
        for (int i = 0; i < 5; ++i)
            yield(i);
    }

    void run()
    {
        boost::coroutines2::coroutine<int>::pull_type source(producer);
        for (const int v : source)
            std::cout << v << ' ';

        // 0 1 2 3 4
    }
}

namespace state_machine
{
    enum class State
    {
        Start,
        Middle,
        End
    };

    void state_machine(boost::coroutines2::coroutine<State>::push_type& yield)
    {
        yield(State::Start);
        yield(State::Middle);
        yield(State::End);
    }

    void run()
    {
        boost::coroutines2::coroutine<State>::pull_type sm(state_machine);
        for (State s : sm)
            std::cout << static_cast<int>(s) << "\n";
    }

    // 0
    // 1
    // 2
}

namespace token_parser
{
    struct Token
    {
        const char* data;
        std::size_t size;
    };

    void token_parser(boost::coroutines2::coroutine<Token>::push_type& yield,
                      const char* buf,
                      const std::size_t len)
    {
        std::size_t start = 0;
        for (std::size_t i = 0; i < len; ++i)
        {
            if (buf[i] == ' ')
            {
                yield({ buf + start, i - start });
                start = i + 1;
            }
        }
        if (start < len)
            yield({ buf + start, len - start });
    }

    void run()
    {
        constexpr char data[] = "FIX SIMPLE PARSER";

        boost::coroutines2::coroutine<Token>::pull_type parser([&](auto& y){
            token_parser(y, data, sizeof(data) - 1);
        });

        for (const Token& t : parser)
        {
            std::cout << t.data << '\n';
        }
    }
}


namespace consumer_demo
{
    void accumulator(boost::coroutines2::coroutine<int>::pull_type& source)
    {
        int sum = 0;

        for (int v : source)
            sum += v;

        std::cout << sum << "\n";
    }

    void run()
    {
        boost::coroutines2::coroutine<int>::push_type sink(accumulator);

        sink(1);
        sink(2);
        sink(3);
    }
}

namespace binary_parser_demo
{
    struct Message
    {
        unsigned char type;
        const char* payload;
        std::size_t size;
    };

    void binary_parser(boost::coroutines2::coroutine<Message>::push_type& yield,
                       const char* buf,
                       const std::size_t len)
    {
        std::size_t i = 0;
        while (i + 2 <= len)
        {
            const unsigned char type = buf[i];
            const unsigned char size = buf[i + 1];

            if (i + 2 + size > len)
                break;

            yield({ type, buf + i + 2, size });
            i += 2 + size;
        }
    }

    void run()
    {
        constexpr char raw[] = {1, 3, 'A', 'B', 'C', 2, 2, 'X', 'Y'};

        boost::coroutines2::coroutine<Message>::pull_type parser([&](auto& y){
            binary_parser(y, raw, sizeof(raw));
        });

        for (const Message& m : parser)
        {
            std::cout << m.type << " " << m.payload << "\n";
        }
    }
}

namespace pipeline_producer_transform
{
    void multiply(boost::coroutines2::coroutine<int>::pull_type& in,
                  boost::coroutines2::coroutine<int>::push_type& out)
    {
        for (const int v : in)
            out(v * 2);
    }

    void run()
    {
        boost::coroutines2::coroutine<int>::push_type output([](auto& in){
            for (int v : in)
                std::cout << v << "\n";
        });

        boost::coroutines2::coroutine<int>::push_type transform([&](auto& in){
            multiply(in, output);
        });

        transform(1);
        transform(2);
        transform(3);
    }
}

int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    // demo_1::run();
    // state_machine::run();
    // token_parser::run();
    // consumer_demo::run();
    // binary_parser_demo::run();
    pipeline_producer_transform::run();



    return EXIT_SUCCESS;
}
