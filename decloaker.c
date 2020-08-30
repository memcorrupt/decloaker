#include <stdio.h>
#include <windows.h>
#include <curl/curl.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

#define assert(x, msg) if(!(x)){ \
printf("ERROR! " msg "\n"); \
getchar(); \
exit(1); \
}

struct curldata {
	char *data;
	size_t size;
};
size_t curlwrite(void *contents, size_t size, size_t nmemb, curldata *data)
{
	size_t sz = size * nmemb;
	data->data = (char *)realloc(data->data, data->size + sz + 1);
	assert(data->data, "out of memory");
	memcpy(data->data + data->size, contents, sz);
	data->size += sz;
	data->data[data->size] = 0;
	return size * nmemb;
}

int main()
{
	unsigned long adapterCount = 0;
	assert(GetAdaptersInfo(NULL, &adapterCount) == ERROR_BUFFER_OVERFLOW, "can't get adapter count");
	IP_ADAPTER_INFO *adapter = (IP_ADAPTER_INFO *)malloc(adapterCount);
	assert(!GetAdaptersInfo(adapter, &adapterCount), "can't get adapters");

	//sort function - https://www.techiedelight.com/given-linked-list-change-sorted-order/
	IP_ADAPTER_INFO *current = adapter;
	IP_ADAPTER_INFO *result = NULL;
	IP_ADAPTER_INFO *next;
	while (current) {
		next = current->Next;
			IP_ADAPTER_INFO dummy;
			IP_ADAPTER_INFO *curr = &dummy;
			dummy.Next = result;

			while (curr->Next && curr->Next->Index < current->Index)
				curr = curr->Next;

			current->Next = curr->Next;
			curr->Next = current;
			result = dummy.Next;
		current = next;
	}
	adapter = result;

	char *interfaceName = NULL;

	while (adapter) {
		if (strcmp(adapter->IpAddressList.IpAddress.String, "0.0.0.0") && memcmp(adapter->Description, "TAP-", 4)) {
			interfaceName = adapter->IpAddressList.IpAddress.String;
			break;
		}
		adapter = adapter->Next;
	}
	assert(interfaceName, "can't get internet connected adapter");

	CURL *curl = curl_easy_init();
	assert(curl, "can't init curl");
	curldata ip = { NULL, 0 };
	curl_easy_setopt(curl, CURLOPT_URL, "http://checkip.amazonaws.com/");
	curl_easy_setopt(curl, CURLOPT_INTERFACE, interfaceName);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlwrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ip);
	assert(curl_easy_perform(curl) == CURLE_OK, "can't send curl request");
	printf("GET DECLOAKED: %s\n", ip.data);
 	getchar();
}
