
#include <sos/sos.h>
#include <sos/crypt_api.h>

const void * kernel_request_api(u32 request){
	switch(request){
		case CRYPT_SHA256_API_REQUEST:
			return &tinycrypt_sha256_hash_api;
	}
	return 0;
}
