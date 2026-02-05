#include <xrpl/basics/LocalValue.h>

namespace xrpl {
namespace detail {

LocalValuesHolder&
getLocalValuesHolder()
{
    thread_local LocalValuesHolder holder;
    return holder;
}

}  // namespace detail
}  // namespace xrpl
