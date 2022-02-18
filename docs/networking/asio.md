# Asio

[Asio](https://think-async.com/Asio/) is the predominant C++ library for asynchronous network programming. Some
applications that use Asio are [libtorrent](http://www.rasterbar.com/products/libtorrent)
, [libbitcoin](http://libbitcoin.org/), [Rippled](https://github.com/ripple/rippled)
, [Restbed](https://github.com/Corvusoft/restbed)
, [AbiWord](http://www.abisource.com/), [Wt](http://www.webtoolkit.eu/wt/), [Dragon](http://dragon.enterasys.com/),
, [reTurn Server](http://www.resiprocate.org/ReTurn_Overview)
, [WebSocket++](https://github.com/zaphoyd/websocketpp)
, [Loggly](http://www.loggly.com/), [BigLog](http://code.google.com/p/biglog/), [Remobo](http://www.remobo.com/)
, [OpenTibia](http://opentibia.sourceforge.net/), [PokerTH](http://www.pokerth.net/)
, [opendnp3](http://www.automatak.com/opendnp3),
[Osiris](http://osiris.kodeware.net/), [P2Engine](https://sourceforge.net/projects/p2engine/)
, [Pion](http://www.pion.org/), [Bit Factory](http://www.bitfactory.at/)
, [CodeShop](http://www.code-shop.com/), [ReSP](http://www.resp-sim.org/), [JukeFly](http://jukefly.com/)
, [QuickFAST](http://quickfast.org/)
, [Rep Invariant JAUS](http://www.repinvariant.com/products/ri-jaus/), [x0](http://xzero.ws/)
, [xiva](http://github.com/highpower/xiva), [Dr.Web](http://products.drweb.com/mailserver/maild/)
, [Swift IM](http://swift.im/), [Blue Gene/Q](https://repo.anl-external.org/repos/bgq-driver/V1R1M2/),
[avhttp](https://github.com/avplayer/avhttp/), [DDT3](http://www.laufenberg.ch/ddt3/)
, [eScada](https://www.escadasolutions.com/), and [ArangoDB](https://github.com/arangodb/arangodb).

Asio is also the basis for
the ["C++ Extensions for Networking"](https://en.cppreference.com/w/cpp/experimental/networking), which benefited from
its 15 years of existing practice.

This library also integrates with Asio to provide interoperability with Asio executors, future types for networking
facilities, and as a form to extend existing Asio applications through future completion tokens.

## Integration

Asio is provided as both a standalone library or as part of the Boost libraries. This library works with both versions
of Asio.

If this library has been integrated through CMake, the build script will already identify and integrate Asio.

When using this library as header-only:

- In C++17, the library can identify the availability of Asio with
  the [`__has_include`](https://en.cppreference.com/w/cpp/preprocessor/include) macro, or...
- The library attempts to identify whether Asio has been included before
  `futures` by checking for the existence of Asio specific macros, or...
- The macros `FUTURES_HAS_ASIO` and `FUTURES_HAS_BOOST_ASIO` can be used to indicate that Asio is available and should
  be included.

If both standalone Asio and Boost.Asio are available, the macros `FUTURES_PREFER_STANDALONE_DEPENDENCIES`
and `FUTURES_PREFER_BOOST_DEPENDENCIES` can be used to indicate which version of Asio should be preferred.

Asio is provided as both a header-only or compiled library. To use the compiled version of Asio, the
macro `ASIO_SEPARATE_COMPILATION` should be defined.

## Asio executors

The Asio library resolves around the `io_context` execution context. An `io_context` can be used as any other executor
in this library to create futures:

{{ code_snippet("networking/asio.cpp", "enqueue") }}

However, the `io_context` executors have two important properties:

1) An `io_context` is a task queue

We need to explicitly ask `io_context` to execute all its pending tasks:

{{ code_snippet("networking/asio.cpp", "pop") }}

Tasks are pushed into the `io_context` queue which prevents them from being executed immediately. By executing the tasks
in the queue, the future value was immediately set.

This effectively defers execution until we pop tasks from the queue. The strategy allows parallel tasks on a single
thread.

2) An `io_context` supports networking tasks

Say we have a server acceptor listening for connections.

{{ code_snippet("networking/asio.cpp", "push_networking") }}

The listening task goes to the `io_context`. By executing the task, we effectively listen for the connections:

{{ code_snippet("networking/asio.cpp", "pop_networking") }}

In a practical application, we would probably handle this connection by pushing more tasks to the executor to read and
write from the client through the client socket.