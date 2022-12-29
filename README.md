# Better Futures

> Because futures don't have to be awful

[![Futures](docs/img/futures_banner.png)](https://alandefreitas.github.io/futures/)

<!--[abstract -->

<br/>

<!--What it is -->

- A *Future* 🔮 is a value to be available an operation fulfills its *Promise* 🤞.

<!--Why this is interesting -->

- The Future/Promise model supports all operations required for async computing: queries, continuations, adaptors, and
  algorithms.

<!--What is the problem -->

- C++11 provides `std::future` but most implementations are useless for efficient applications.

<!--Why is it unsolved -->

- There are countless proposals to improve this C++11 component: continuations, cancellation, executors, and algorithms.

<!--What is the solution -->

- This library provides a concept to integrate existing applications and new improved future types.

<!--What the solution achieves -->

- This design allows the library to include generic algorithms, executors, adaptors, and custom extensions.

<br/>

<div style="text-align: center;">
<a href="https://github.com/alandefreitas/futures/actions?query=workflow%3ABuild+event%3Apush+branch%3Amaster+" target="_blank">
  <img alt="Build Status" src="https://img.shields.io/github/actions/workflow/status/alandefreitas/futures/build.yml?branch=master&label=Build&logo=Github-Actions&event=push">
</a>
<a href="https://GitHub.com/alandefreitas/futures/releases/" target="_blank">
  <img alt="Latest Release" src="https://img.shields.io/github/release/alandefreitas/futures.svg?label=Download">
</a>
<a href="https://alandefreitas.github.io/futures/" target="_blank">
  <img alt="Documentation" src="https://img.shields.io/website-up-down-green-red/http/alandefreitas.github.io/futures.svg?label=Documentation">
</a>
<a href="https://github.com/alandefreitas/futures/discussions" target="_blank">
  <img alt="Discussions" src="https://img.shields.io/website-up-down-green-red/http/alandefreitas.github.io/futures.svg?label=Discussions">
</a>
<a href="https://codecov.io/gh/alandefreitas/futures"  target="_blank">
  <img alt="Coverage" src="https://codecov.io/gh/alandefreitas/futures/branch/master/graph/badge.svg?token=ZW86JQCVYI"/> 
</a>
</div>

<br/>

<div style="text-align: center;">

<!-- https://github.com/bradvin/social-share-urls -->

<a href="https://www.facebook.com/sharer/sharer.php?t=futures:%20C%2B%2B%20Task%20Programming&u=https://github.com/alandefreitas/futures/" target="_blank">
    <img alt="Facebook" src="https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+Facebook&logo=facebook">
</a>
<a href="http://sns.qzone.qq.com/cgi-bin/qzshare/cgi_qzshare_onekey?url=https://github.com/alandefreitas/futures/&title=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors&summary=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors" target="_blank">
    <img alt="QZone" src="https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+QZone&logo=qzone">
</a>
<a href="http://sns.qzone.qq.com/cgi-bin/qzshare/cgi_qzshare_onekey?url=https://github.com/alandefreitas/futures/&title=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors&summary=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors" target="_blank">
    <img alt="Weibo" src="https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+Weibo&logo=sina-weibo">
</a>
<a href="http://www.reddit.com/submit?url=https://github.com/alandefreitas/futures/&title=Futures:%20CPP%20Task%20Programming%20with%20Asio%20Executors" target="_blank">
    <img alt="Reddit" src="https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+Reddit&logo=reddit">
</a>
<a href="https://twitter.com/intent/tweet?text=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors&url=https://github.com/alandefreitas/futures/&hashtags=Task,Programming,Cpp,Async" target="_blank">
    <img alt="Twitter" src="https://img.shields.io/twitter/url/http/shields.io.svg?label=Share+on+Twitter&style=social">
</a>
<a href="https://www.linkedin.com/shareArticle?mini=false&url=https://github.com/alandefreitas/futures/&title=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors" target="_blank">
    <img alt="LinkedIn" src="https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+LinkedIn&logo=linkedin">
</a>
<a href="https://api.whatsapp.com/send?text=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors:+https://github.com/alandefreitas/futures/" target="_blank">
    <img alt="WhatsApp" src="https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+WhatsApp&logo=whatsapp">
</a>
<a href="https://lineit.line.me/share/ui?url=https://github.com/alandefreitas/futures/&text=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors" target="_blank">
    <img alt="Line.me" src="https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+Line.me&logo=line">
</a>
<a href="https://telegram.me/share/url?url=https://github.com/alandefreitas/futures/&text=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors" target="_blank">
    <img alt="Telegram.me" src="https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+Telegram.me&logo=telegram">
</a>
<a href="https://news.ycombinator.com/submitlink?u=https://github.com/alandefreitas/futures/&t=futures:%20C%2B%2B%20task%20programming%20with%20asio%20executors" target="_blank">
    <img alt="HackerNews" src="https://img.shields.io/twitter/url/http/shields.io.svg?style=social&label=Share+on+HackerNews&logo=y-combinator">
</a>

</div>

<br/>

<!--] -->

<br/>

<div style="text-align: center;">

<h2>

<a href="https://alandefreitas.github.io/futures/">
  <img src="https://upload.wikimedia.org/wikipedia/commons/2/2a/Documentation-plain.svg" width="50%"/>
</a>

[READ THE DOCUMENTATION FOR A QUICK START AND EXAMPLES](https://alandefreitas.github.io/futures/)

</h2>

</div>


