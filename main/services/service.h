#ifndef __SERVICE_H__
#define __SERVICE_H__

#include <stdint.h>
#include <stddef.h>
#include <esp_log.h>

enum service_Status
{
    SERVICE_STATUS_OK = 0,
    SERVICE_STATUS_ERROR,
    SERVICE_STATUS_MAX,
};

typedef uint8_t service_Status_t;

/**
 * \brief  This macro enables accessing a parent structure
 *          from a pointer to a contained member.
 *
 * \param ptr pointer to a member within the structure.
 * \param type type of the container structure.
 * \param ptr the name of the member within the structure.
 * \example
 * \code
 *
 *  typedef struct {
 *      BaseA baseA;  // First "parent"
 *      BaseB baseB;  // Second "parent"
 *   } Derived;
 *
 *   void foo( BaseA *a,BaseB *b)
 *   {
 *      Derived* derived_from_baseA = service_CONTAINER_OF(a, Derived, baseA);
 *      Derived* derived_from_baseB = service_CONTAINER_OF(b, Derived, baseB);
 *   }
 *
 * \endcode
 */
#define service_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define SERVICE_LOGE(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)

#define SERVICE_LOGD(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)

#define SERVICE_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)

#endif //__SERVICE_H__