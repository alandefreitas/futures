#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    {
        //[create_shared Creating a shared future
        cfuture<int> f1 = async([] { return 1; });
        shared_cfuture<int> f2 = f1.share();
        //]
    }

    {
        //[invalidate_unique Previous future is invalidated
        cfuture<int> f1 = async([] { return 1; });
        shared_cfuture<int> f2 = f1.share();
        std::cout << f1.valid() << '\n'; // returns false
        std::cout << f2.valid() << '\n'; // returns true
        //]
    }

    {
        //[single_step Creating a shared future
        shared_cfuture<int> f = async([] { return 1; }).share();
        std::cout << f.get() << '\n'; // returns 1
        //]
    }

    //[share_state Sharing the future state
    shared_cfuture<int> f1 = async([] { return 1; }).share();

    // OK to copy
    shared_cfuture<int> f2 = f1;
    //]

    //[get_state Sharing the future state
    // OK to get
    std::cout << f1.get() << '\n';

    // OK to call get on the copy
    std::cout << f2.get() << '\n';

    // OK to call get twice
    std::cout << f1.get() << '\n';
    std::cout << f2.get() << '\n';
    //]

    {
        {
            //[future_vector Future vector (value is moved)
            cfuture<std::vector<int>> f = async([] {
                return std::vector<int>(1000, 0);
            });
            std::vector<int> v = f.get();   // value is moved
            std::cout << f.valid() << '\n'; // future is now invalid
            //]
        }

        {
            //[shared_future_vector Shared vector (value is copied)
            shared_cfuture<std::vector<int>>
                f = async([] {
                        return std::vector<int>(1000, 0);
                    }).share();
            std::vector<int> v = f.get();   // value is copied
            std::cout << f.valid() << '\n'; // future is still valid
            std::vector<int> v2 = f.get();  // value is copied again
            //]
        }
    }

    return 0;
}