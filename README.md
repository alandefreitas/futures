# Futures

> C++ Task Programming with Asio Executors

[![Futures](docs/img/futures_banner.png)](https://alandefreitas.github.io/futures/)

<br/>

- A `future` object represents a handle to a value that might not be available yet from an asynchronous operation.
- To compose with multiple tasks, they allow us to query whether this future value is ready and to obtain its value once it is made available by some provider.
- A number of proposals have been presented as extend this model in C++, such as future continuations, cancellation tokens, association with executors, and algorithms.  
- Rather than proposing one more concrete future type, this library implements a number of types that follow a future *concept*, which also includes existing future types, such as `std::future` and `boost::future`.  
- The concepts allow reusable algorithms for all future types, an alternative to `std::async` based on executors, various efficient future types, many future composition algorithms, a syntax closer to other programming languages, and parallel variants of the STL algorithms.

<br/>

[![Build Status](https://img.shields.io/github/workflow/status/alandefreitas/futures/Build?event=push&label=Build&logo=Github-Actions)](https://github.com/alandefreitas/futures/actions?query=workflow%3ABuild+event%3Apush)
[![Latest Release](https://img.shields.io/github/release/alandefreitas/futures.svg?label=Download)](https://GitHub.com/alandefreitas/futures/releases/)
[![Documentation](https://img.shields.io/website-up-down-green-red/http/alandefreitas.github.io/futures.svg?label=Documentation)](https://alandefreitas.github.io/futures/)
[![Discussions](https://img.shields.io/website-up-down-green-red/http/alandefreitas.github.io/futures.svg?label=Discussions)](https://github.com/alandefreitas/futures/discussions)

<br/>

<!-- https://github.com/bradvin/social-share-urls -->
[![Facebook](https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+Facebook&logo=facebook)](https://www.facebook.com/sharer/sharer.php?t=futures:%20C%2B%2B%20Task%20Programming&u=https://github.com/alandefreitas/futures/)
[![QZone](https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+QZone&logo=qzone)](http://sns.qzone.qq.com/cgi-bin/qzshare/cgi_qzshare_onekey?url=https://github.com/alandefreitas/futures/&title=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors&summary=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors)
[![Weibo](https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+Weibo&logo=sina-weibo)](http://sns.qzone.qq.com/cgi-bin/qzshare/cgi_qzshare_onekey?url=https://github.com/alandefreitas/futures/&title=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors&summary=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors)
[![Reddit](https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+Reddit&logo=reddit)](http://www.reddit.com/submit?url=https://github.com/alandefreitas/futures/&title=Futures:%20CPP%20Task%20Programming%20with%20Asio%20Executors)
[![Twitter](https://img.shields.io/twitter/url/http/shields.io.svg?label=Share+on+Twitter&style=social)](https://twitter.com/intent/tweet?text=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors&url=https://github.com/alandefreitas/futures/&hashtags=Task,Programming,Cpp,Async)
[![LinkedIn](https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+LinkedIn&logo=linkedin)](https://www.linkedin.com/shareArticle?mini=false&url=https://github.com/alandefreitas/futures/&title=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors)
[![WhatsApp](https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+WhatsApp&logo=whatsapp)](https://api.whatsapp.com/send?text=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors:+https://github.com/alandefreitas/futures/)
[![Line.me](https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+Line.me&logo=line)](https://lineit.line.me/share/ui?url=https://github.com/alandefreitas/futures/&text=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors)
[![Telegram.me](https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+Telegram.me&logo=telegram)](https://telegram.me/share/url?url=https://github.com/alandefreitas/futures/&text=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors)
[![HackerNews](https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+HackerNews&logo=y-combinator)](https://news.ycombinator.com/submitlink?u=https://github.com/alandefreitas/futures/&t=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors)

<br/>

<h2>

[READ THE DOCUMENTATION FOR A QUICK START AND EXAMPLES](https://alandefreitas.github.io/futures/)

[![Futures](https://upload.wikimedia.org/wikipedia/commons/2/2a/Documentation-plain.svg)](https://alandefreitas.github.io/futures/)

</h2>


