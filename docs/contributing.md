## Roadmap 🛣

- Additional future types and options
- Additional launching policies
- Additional future adaptors
- Additional algorithms
- Additional executors

## Guidelines 📐

* Discussions are concentrated on our GitHub [discussions](https://github.com/alandefreitas/futures/discussions) page. Don't refrain from asking questions and proposing ideas. If this library helps you create something interesting, please divulge it with the community.
* If you are a programmer with good ideas, please [share](https://github.com/alandefreitas/futures/discussions/new) these ideas with us.
* Academic collaboration is more than welcome. It'd be great to see this library help people write papers.
* If contributing with code, please leave all warnings ON (`-DFUTURES_BUILD_WITH_PEDANTIC_WARNINGS=ON`), use [cppcheck](http://cppcheck.sourceforge.net/), and [clang-format](https://clang.llvm.org/docs/ClangFormat.html).

## Ideas 💡

Feel free to contribute new features to this library. For complex features and changes, consider [getting feedback](https://github.com/alandefreitas/futures/discussions/new) from the community first. Contributing to an existing code base with its conventions might seem obscure at first but please don't let that discourage you from sharing your ideas.

There are many ways in which you can contribute to this library:

* Testing the library in new environments <sup>see [1](https://github.com/alandefreitas/futures/issues?q=is%3Aopen+is%3Aissue+label%3A%22cross-platform+issue+-+windows%22), [2](https://github.com/alandefreitas/futures/issues?q=is%3Aopen+is%3Aissue+label%3A%22cross-platform+issue+-+linux%22), [3](https://github.com/alandefreitas/futures/issues?q=is%3Aopen+is%3Aissue+label%3A%22cross-platform+issue+-+macos%22) </sup>
* Contributing with interesting examples <sup>see [1](source/examples)</sup>
* Finding problems in this documentation <sup>see [1](https://github.com/alandefreitas/futures/issues?q=is%3Aopen+is%3Aissue+label%3A%22enhancement+-+documentation%22) </sup>
* Finding bugs in general <sup>see [1](https://github.com/alandefreitas/futures/issues?q=is%3Aopen+is%3Aissue+label%3A%22bug+-+compilation+error%22), [2](https://github.com/alandefreitas/futures/issues?q=is%3Aopen+is%3Aissue+label%3A%22bug+-+compilation+warning%22), [3](https://github.com/alandefreitas/futures/issues?q=is%3Aopen+is%3Aissue+label%3A%22bug+-+runtime+error%22), [4](https://github.com/alandefreitas/futures/issues?q=is%3Aopen+is%3Aissue+label%3A%22bug+-+runtime+warning%22) </sup>
* Whatever idea seems interesting to you

The only thing we ask you is to make sure your contribution is not destructive. Some contributions in which we are not interested are:

* "I don't like this optional feature, so I removed/deprecated it"
* "I removed this feature to support older versions of C++" but have not provided an equivalent alternative
* "I removed this feature, so I don't have to install/update ______" but have not provided an equivalent alternative
* "I'm creating this high-cost promise that we'll support ________ forever" but I'm not sticking around to keep that promise

In doubt, please open a [discussion](https://github.com/alandefreitas/futures/discussions) first

--8<-- "docs/references.md"