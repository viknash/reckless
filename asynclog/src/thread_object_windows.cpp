#include "asynclog/detail/thread_object.hpp"

asynclog::detail::tls_key_t asynclog::detail::create_tls_key(void (CALLBACK *pdestroy)(void*))
{
	tls_key_t key = FlsAlloc(pdestroy);
	if(key == FLS_OUT_OF_INDEXES)
		throw std::bad_alloc();
}