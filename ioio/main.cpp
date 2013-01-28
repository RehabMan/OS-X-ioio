//
//  main.cpp
//  ioio
//
//  Created by RehabMan on 1/5/13.
//  Copyright (c) 2013 RehabMan. All rights reserved.
//
//  A simple command line app to tweak kext/driver params.
//

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/usb/IOUSBLib.h>
#include <mach/mach.h>
#include <unistd.h>
#include <string.h>

#ifdef DEBUG
#define DEBUG_LOG(args...)   do { printf(args); fflush(stdout); } while (0)
#else
#define DEBUG_LOG(args...)   do { } while (0)
#endif

static void SetNumberProperty(io_service_t service, const char* property, int64_t value)
{
    assert(service);
    
    CFStringRef cf_key = CFStringCreateWithCString(kCFAllocatorDefault, property, CFStringGetSystemEncoding());
    CFNumberRef cf_number = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &value);
    kern_return_t kr = IORegistryEntrySetCFProperty(service, cf_key, cf_number);
    if (KERN_SUCCESS != kr)
        DEBUG_LOG("IORegistryEntrySetCFProperty() returned error 0x%08x\n", kr);
    CFRelease(cf_key);
    CFRelease(cf_number);
}

static void SetBoolProperty(io_service_t service, const char* property, bool value)
{
    assert(service);
    
    CFStringRef cf_key = CFStringCreateWithCString(kCFAllocatorDefault, property, CFStringGetSystemEncoding());
    kern_return_t kr = IORegistryEntrySetCFProperty(service, cf_key, value ? kCFBooleanTrue : kCFBooleanFalse);
    if (KERN_SUCCESS != kr)
        DEBUG_LOG("IORegistryEntrySetCFProperty() returned error 0x%08x\n", kr);
    CFRelease(cf_key);
}

static void SetStringProperty(io_service_t service, const char* property, const char* value)
{
    assert(service);
    
    CFStringRef cf_key = CFStringCreateWithCString(kCFAllocatorDefault, property, CFStringGetSystemEncoding());
    CFStringRef cf_value = CFStringCreateWithCString(kCFAllocatorDefault, value, CFStringGetSystemEncoding());
    kern_return_t kr = IORegistryEntrySetCFProperty(service, cf_key, cf_value);
    if (KERN_SUCCESS != kr)
        DEBUG_LOG("IORegistryEntrySetCFProperty() returned error 0x%08x\n", kr);
    CFRelease(cf_key);
    CFRelease(cf_value);
}



void print_usage()
{
    static const char* usage =
        "Usage: ioio [options] <name> <value>\n"
        "Options:\n"
        "  -s <service> (default service is ApplePS2SynapticsTouchPad)\n"
        "  -h prints this messsage\n"
        "\n"
        "<name> is name of property to set (use quotes if it has spaces)\n"
        "<value> is value to set\n"
        "  use true, false, yes, or no for boolean properties\n"
        "  use non-digits for strings\n"
        "  for string starting with a digit prefix string with :\n"
        "  use digits for decimal number\n"
        "\n";
    printf("%s", usage);
}


int main(int argc, const char * argv[])
{
    const char* szService = "ApplePS2SynapticsTouchPad";

#ifdef DEBUG
    for (int i = 0; i < argc; i++)
        printf("argv[%d] = { %s }\n", i, argv[i]);
#endif
    
    // parse args
    const char** argv_new = new const char*[argc];
    int argc_new = 0;
    for (int i = 1; i < argc; i++)
    {
        if (*argv[i] != '-')
        {
            argv_new[argc_new++] = argv[i];
            continue;
        }
        switch (argv[i][1])
        {
            case 'h':
                print_usage();
                exit(-1);
                break;
                
            case 's':
                ++i;
                if (i >= argc)
                {
                    print_usage();
                    exit(-1);
                }
                szService = argv[i];
                break;
                
            default:
                argv_new[argc_new++] = argv[i];
                break;
        }
    }
    if (argc_new != 2)
    {
        print_usage();
        exit(-1);
    }
    const char* szName = argv_new[0];
    const char* szValue = argv_new[1];
    
    // look up service
    io_service_t service = IOServiceGetMatchingService(0, IOServiceMatching(szService));
	if (!service)
	{
        printf("No service matching \"%s\"\n", szService);
        exit(-1);
	}

    // set value
    if (strcmp(szValue, "yes") == 0 || strcmp(szValue, "true") == 0)
    {
        printf("ioio: setting property '%s:%s' as boolean to true\n", szService, szName);
        SetBoolProperty(service, szName, true);
    }
    else if (strcmp(szValue, "no") == 0 || strcmp(szValue, "false") == 0)
    {
        printf("ioio: setting property '%s:%s' as boolean to false\n", szService, szName);
        SetBoolProperty(service, szName, false);
    }
    else if (!isdigit(*szValue) && *szValue != '-')
    {
        if (*szValue == ':')
            ++szValue;
        printf("ioio: setting property '%s:%s' as string to '%s'\n", szService, szName, szValue);
        SetStringProperty(service, szName, szValue);
    }
    else
    {
        long long value;
        if (szValue[0] == '0' && (szValue[1] == 'x' || szValue[1] == 'X'))
            sscanf(szValue, "%llx", &value);
        else
            value = atoll(szValue);
        printf("ioio: setting property '%s:%s' as number to %lld (0x%llx)\n", szService, szName, value, value);
        SetNumberProperty(service, szName, value);
    }
    
    IOObjectRelease(service);
}
