#include <array>
#include <catch2/catch.hpp>
#include <futures/futures.h>
#if __has_include(<futures/algorithm.h>)
#include <futures/algorithm.h>
#endif

TEST_CASE(TEST_CASE_PREFIX "Async algorithm") {
    using namespace futures;

    auto ex = make_default_executor();

    std::vector<int> v(5000);
    std::iota(v.begin(), v.end(), 1);
    constexpr int v_sum = 12502500;
    static int v_prod = std::accumulate(v.begin(), v.end(), 1, std::multiplies<>());
    auto p = [](std::vector<int>::iterator first, std::vector<int>::iterator last) {
        return std::next(first, (last - first) / 2);
    };

    SECTION("for_each") {
        std::mutex m;
        int count_ = 0;
        auto fun = [&](int v) {
            std::lock_guard lk{m};
            count_ += v;
        };

        SECTION("Basic") {
            SECTION("Ranges") {
                for_each(v, fun);
                REQUIRE(count_ == v_sum);
            }

            SECTION("Iterators") {
                for_each(v.begin(), v.end(), fun);
                REQUIRE(count_ == v_sum);
            }
        }

        SECTION("Custom executor") {
            SECTION("Ranges") {
                for_each(ex, v, fun);
                REQUIRE(count_ == v_sum);
            }

            SECTION("Iterators") {
                for_each(ex, v.begin(), v.end(), fun);
                REQUIRE(count_ == v_sum);
            }
        }

        SECTION("Policy") {
            SECTION("Ranges") {
                for_each(seq, v, fun);
                REQUIRE(count_ == v_sum);
            }

            SECTION("Iterators") {
                for_each(seq, v.begin(), v.end(), fun);
                REQUIRE(count_ == v_sum);
            }
        }

        SECTION("Custom partitioner") {
            SECTION("Ranges") {
                for_each(p, v, fun);
                REQUIRE(count_ == v_sum);
            }

            SECTION("Iterators") {
                for_each(p, v.begin(), v.end(), fun);
                REQUIRE(count_ == v_sum);
            }
        }

        SECTION("Custom executor and partitioner") {
            SECTION("Ranges") {
                for_each(ex, p, v, fun);
                REQUIRE(count_ == v_sum);
            }

            SECTION("Iterators") {
                for_each(ex, p, v.begin(), v.end(), fun);
                REQUIRE(count_ == v_sum);
            }
        }
    }

    SECTION("all_of") {
        auto fun = [&](int v) { return v < 5500; };

        SECTION("Basic") {
            SECTION("Ranges") { REQUIRE(all_of(v, fun)); }

            SECTION("Iterators") { REQUIRE(all_of(v.begin(), v.end(), fun)); }
        }

        SECTION("Custom executor") {
            SECTION("Ranges") { REQUIRE(all_of(ex, v, fun)); }

            SECTION("Iterators") { REQUIRE(all_of(ex, v.begin(), v.end(), fun)); }
        }

        SECTION("Policy") {
            SECTION("Ranges") { REQUIRE(all_of(seq, v, fun)); }

            SECTION("Iterators") { REQUIRE(all_of(seq, v.begin(), v.end(), fun)); }
        }

        SECTION("Custom partitioner") {
            SECTION("Ranges") { REQUIRE(all_of(p, v, fun)); }

            SECTION("Iterators") { REQUIRE(all_of(p, v.begin(), v.end(), fun)); }
        }

        SECTION("Custom executor and partitioner") {
            SECTION("Ranges") { REQUIRE(all_of(ex, p, v, fun)); }

            SECTION("Iterators") { REQUIRE(all_of(ex, p, v.begin(), v.end(), fun)); }
        }
    }

    SECTION("any_of") {
        auto fun = [&](int v) { return v == 2700; };

        SECTION("Basic") {
            SECTION("Ranges") { REQUIRE(any_of(v, fun)); }

            SECTION("Iterators") { REQUIRE(any_of(v.begin(), v.end(), fun)); }
        }

        SECTION("Custom executor") {
            SECTION("Ranges") { REQUIRE(any_of(ex, v, fun)); }

            SECTION("Iterators") { REQUIRE(any_of(ex, v.begin(), v.end(), fun)); }
        }

        SECTION("Policy") {
            SECTION("Ranges") { REQUIRE(any_of(seq, v, fun)); }

            SECTION("Iterators") { REQUIRE(any_of(seq, v.begin(), v.end(), fun)); }
        }

        SECTION("Custom partitioner") {
            SECTION("Ranges") { REQUIRE(any_of(p, v, fun)); }

            SECTION("Iterators") { REQUIRE(any_of(p, v.begin(), v.end(), fun)); }
        }

        SECTION("Custom executor and partitioner") {
            SECTION("Ranges") { REQUIRE(any_of(ex, p, v, fun)); }

            SECTION("Iterators") { REQUIRE(any_of(ex, p, v.begin(), v.end(), fun)); }
        }
    }

    SECTION("none_of") {
        auto fun = [&](int v) { return v > 5500; };

        SECTION("Basic") {
            SECTION("Ranges") { REQUIRE(none_of(v, fun)); }

            SECTION("Iterators") { REQUIRE(none_of(v.begin(), v.end(), fun)); }
        }

        SECTION("Custom executor") {
            SECTION("Ranges") { REQUIRE(none_of(ex, v, fun)); }

            SECTION("Iterators") { REQUIRE(none_of(ex, v.begin(), v.end(), fun)); }
        }

        SECTION("Policy") {
            SECTION("Ranges") { REQUIRE(none_of(seq, v, fun)); }

            SECTION("Iterators") { REQUIRE(none_of(seq, v.begin(), v.end(), fun)); }
        }

        SECTION("Custom partitioner") {
            SECTION("Ranges") { REQUIRE(none_of(p, v, fun)); }

            SECTION("Iterators") { REQUIRE(none_of(p, v.begin(), v.end(), fun)); }
        }

        SECTION("Custom executor and partitioner") {
            SECTION("Ranges") { REQUIRE(none_of(ex, p, v, fun)); }

            SECTION("Iterators") { REQUIRE(none_of(ex, p, v.begin(), v.end(), fun)); }
        }
    }

    SECTION("find") {
        int t = 2700;

        SECTION("Basic") {
            SECTION("Ranges") { REQUIRE(find(v, t) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find(v.begin(), v.end(), t) == v.begin() + 2699); }
        }

        SECTION("Custom executor") {
            SECTION("Ranges") { REQUIRE(find(ex, v, t) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find(ex, v.begin(), v.end(), t) == v.begin() + 2699); }
        }

        SECTION("Policy") {
            SECTION("Ranges") { REQUIRE(find(seq, v, t) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find(seq, v.begin(), v.end(), t) == v.begin() + 2699); }
        }

        SECTION("Custom partitioner") {
            SECTION("Ranges") { REQUIRE(find(p, v, t) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find(p, v.begin(), v.end(), t) == v.begin() + 2699); }
        }

        SECTION("Custom executor and partitioner") {
            SECTION("Ranges") { REQUIRE(find(ex, p, v, t) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find(ex, p, v.begin(), v.end(), t) == v.begin() + 2699); }
        }
    }

    SECTION("find_if") {
        auto fun = [&](int v) { return v >= 2700; };

        SECTION("Basic") {
            SECTION("Ranges") { REQUIRE(find_if(v, fun) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find_if(v.begin(), v.end(), fun) == v.begin() + 2699); }
        }

        SECTION("Custom executor") {
            SECTION("Ranges") { REQUIRE(find_if(ex, v, fun) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find_if(ex, v.begin(), v.end(), fun) == v.begin() + 2699); }
        }

        SECTION("Policy") {
            SECTION("Ranges") { REQUIRE(find_if(seq, v, fun) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find_if(seq, v.begin(), v.end(), fun) == v.begin() + 2699); }
        }

        SECTION("Custom partitioner") {
            SECTION("Ranges") { REQUIRE(find_if(p, v, fun) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find_if(p, v.begin(), v.end(), fun) == v.begin() + 2699); }
        }

        SECTION("Custom executor and partitioner") {
            SECTION("Ranges") { REQUIRE(find_if(ex, p, v, fun) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find_if(ex, p, v.begin(), v.end(), fun) == v.begin() + 2699); }
        }
    }

    SECTION("find_if_not") {
        auto fun = [&](int v) { return v < 2700; };

        SECTION("Basic") {
            SECTION("Ranges") { REQUIRE(find_if_not(v, fun) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find_if_not(v.begin(), v.end(), fun) == v.begin() + 2699); }
        }

        SECTION("Custom executor") {
            SECTION("Ranges") { REQUIRE(find_if_not(ex, v, fun) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find_if_not(ex, v.begin(), v.end(), fun) == v.begin() + 2699); }
        }

        SECTION("Policy") {
            SECTION("Ranges") { REQUIRE(find_if_not(seq, v, fun) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find_if_not(seq, v.begin(), v.end(), fun) == v.begin() + 2699); }
        }

        SECTION("Custom partitioner") {
            SECTION("Ranges") { REQUIRE(find_if_not(p, v, fun) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find_if_not(p, v.begin(), v.end(), fun) == v.begin() + 2699); }
        }

        SECTION("Custom executor and partitioner") {
            SECTION("Ranges") { REQUIRE(find_if_not(ex, p, v, fun) == v.begin() + 2699); }

            SECTION("Iterators") { REQUIRE(find_if_not(ex, p, v.begin(), v.end(), fun) == v.begin() + 2699); }
        }
    }

    SECTION("count") {
        int t = 2000;

        SECTION("Basic") {
            SECTION("Ranges") {
                size_t c = count(v, t);
                REQUIRE(c == 1);
            }

            SECTION("Iterators") {
                size_t c = count(v.begin(), v.end(), t);
                REQUIRE(c == 1);
            }
        }

        SECTION("Custom executor") {
            SECTION("Ranges") {
                size_t c = count(ex, v, t);
                REQUIRE(c == 1);
            }

            SECTION("Iterators") {
                size_t c = count(ex, v.begin(), v.end(), t);
                REQUIRE(c == 1);
            }
        }

        SECTION("Policy") {
            SECTION("Ranges") {
                size_t c = count(seq, v, t);
                REQUIRE(c == 1);
            }

            SECTION("Iterators") {
                size_t c = count(seq, v.begin(), v.end(), t);
                REQUIRE(c == 1);
            }
        }

        SECTION("Custom partitioner") {
            SECTION("Ranges") {
                size_t c = count(p, v, t);
                REQUIRE(c == 1);
            }

            SECTION("Iterators") {
                size_t c = count(p, v.begin(), v.end(), t);
                REQUIRE(c == 1);
            }
        }

        SECTION("Custom executor and partitioner") {
            SECTION("Ranges") {
                size_t c = count(ex, p, v, t);
                REQUIRE(c == 1);
            }

            SECTION("Iterators") {
                size_t c = count(ex, p, v.begin(), v.end(), t);
                REQUIRE(c == 1);
            }
        }
    }

    SECTION("count_if") {
        auto fun = [&](int v) { return v & 1; };

        SECTION("Basic") {
            SECTION("Ranges") {
                size_t c = count_if(v, fun);
                REQUIRE(c == 2500);
            }

            SECTION("Iterators") {
                size_t c = count_if(v.begin(), v.end(), fun);
                REQUIRE(c == 2500);
            }
        }

        SECTION("Custom executor") {
            SECTION("Ranges") {
                size_t c = count_if(ex, v, fun);
                REQUIRE(c == 2500);
            }

            SECTION("Iterators") {
                size_t c = count_if(ex, v.begin(), v.end(), fun);
                REQUIRE(c == 2500);
            }
        }

        SECTION("Policy") {
            SECTION("Ranges") {
                size_t c = count_if(seq, v, fun);
                REQUIRE(c == 2500);
            }

            SECTION("Iterators") {
                size_t c = count_if(seq, v.begin(), v.end(), fun);
                REQUIRE(c == 2500);
            }
        }

        SECTION("Custom partitioner") {
            SECTION("Ranges") {
                size_t c = count_if(p, v, fun);
                REQUIRE(c == 2500);
            }

            SECTION("Iterators") {
                size_t c = count_if(p, v.begin(), v.end(), fun);
                REQUIRE(c == 2500);
            }
        }

        SECTION("Custom executor and partitioner") {
            SECTION("Ranges") {
                size_t c = count_if(ex, p, v, fun);
                REQUIRE(c == 2500);
            }

            SECTION("Iterators") {
                size_t c = count_if(ex, p, v.begin(), v.end(), fun);
                REQUIRE(c == 2500);
            }
        }
    }

    SECTION("reduce") {
        SECTION("Default initial value") {
            SECTION("Basic") {
                SECTION("Plus") {
                    SECTION("Ranges") {
                        int count_ = reduce(v);
                        REQUIRE(count_ == v_sum);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(v.begin(), v.end());
                        REQUIRE(count_ == v_sum);
                    }
                }

                SECTION("Custom function") {
                    SECTION("Ranges") {
                        int count_ = reduce(v, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(v.begin(), v.end(), std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }
                }
            }

            SECTION("Custom executor") {
                SECTION("Plus") {
                    SECTION("Ranges") {
                        int count_ = reduce(ex, v);
                        REQUIRE(count_ == v_sum);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(ex, v.begin(), v.end());
                        REQUIRE(count_ == v_sum);
                    }
                }

                SECTION("Custom function") {
                    SECTION("Ranges") {
                        int count_ = reduce(v, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(v.begin(), v.end(), std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }
                }
            }

            SECTION("Policy") {
                SECTION("Plus") {
                    SECTION("Ranges") {
                        int count_ = reduce(seq, v);
                        REQUIRE(count_ == v_sum);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(seq, v.begin(), v.end());
                        REQUIRE(count_ == v_sum);
                    }
                }

                SECTION("Custom function") {
                    SECTION("Ranges") {
                        int count_ = reduce(seq, v, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(seq, v.begin(), v.end(), std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }
                }
            }

            SECTION("Custom partitioner") {
                SECTION("Plus") {
                    SECTION("Ranges") {
                        int count_ = reduce(p, v);
                        REQUIRE(count_ == v_sum);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(p, v.begin(), v.end());
                        REQUIRE(count_ == v_sum);
                    }
                }

                SECTION("Custom function") {
                    SECTION("Ranges") {
                        int count_ = reduce(p, v, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(p, v.begin(), v.end(), std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }
                }
            }

            SECTION("Custom executor and partitioner") {
                SECTION("Plus") {
                    SECTION("Ranges") {
                        int count_ = reduce(ex, p, v);
                        REQUIRE(count_ == v_sum);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(ex, p, v.begin(), v.end());
                        REQUIRE(count_ == v_sum);
                    }
                }

                SECTION("Custom function") {
                    SECTION("Ranges") {
                        int count_ = reduce(ex, p, v, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(ex, p, v.begin(), v.end(), std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }
                }
            }
        }

        SECTION("Custom initial value") {
            SECTION("Basic") {
                SECTION("Plus") {
                    SECTION("Ranges") {
                        int count_ = reduce(v, 0);
                        REQUIRE(count_ == v_sum);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(v.begin(), v.end(), 0);
                        REQUIRE(count_ == v_sum);
                    }
                }

                SECTION("Custom function") {
                    SECTION("Ranges") {
                        int count_ = reduce(v, 1, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(v.begin(), v.end(), 1, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }
                }
            }

            SECTION("Custom executor") {
                SECTION("Plus") {
                    SECTION("Ranges") {
                        int count_ = reduce(ex, v, 0);
                        REQUIRE(count_ == v_sum);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(ex, v.begin(), v.end(), 0);
                        REQUIRE(count_ == v_sum);
                    }
                }

                SECTION("Custom function") {
                    SECTION("Ranges") {
                        int count_ = reduce(v, 1, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(v.begin(), v.end(), 1, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }
                }
            }

            SECTION("Policy") {
                SECTION("Plus") {
                    SECTION("Ranges") {
                        int count_ = reduce(seq, v, 0);
                        REQUIRE(count_ == v_sum);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(seq, v.begin(), v.end(), 0);
                        REQUIRE(count_ == v_sum);
                    }
                }

                SECTION("Custom function") {
                    SECTION("Ranges") {
                        int count_ = reduce(seq, v, 1, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(seq, v.begin(), v.end(), 1, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }
                }
            }

            SECTION("Custom partitioner") {
                SECTION("Plus") {
                    SECTION("Ranges") {
                        int count_ = reduce(p, v, 0);
                        REQUIRE(count_ == v_sum);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(p, v.begin(), v.end(), 0);
                        REQUIRE(count_ == v_sum);
                    }
                }

                SECTION("Custom function") {
                    SECTION("Ranges") {
                        int count_ = reduce(p, v, 1, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(p, v.begin(), v.end(), 1, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }
                }
            }

            SECTION("Custom executor and partitioner") {
                SECTION("Plus") {
                    SECTION("Ranges") {
                        int count_ = reduce(ex, p, v, 0);
                        REQUIRE(count_ == v_sum);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(ex, p, v.begin(), v.end(), 0);
                        REQUIRE(count_ == v_sum);
                    }
                }

                SECTION("Custom function") {
                    SECTION("Ranges") {
                        int count_ = reduce(ex, p, v, 1, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }

                    SECTION("Iterators") {
                        int count_ = reduce(ex, p, v.begin(), v.end(), 1, std::multiplies<>());
                        REQUIRE(count_ == v_prod);
                    }
                }
            }
        }
    }

}