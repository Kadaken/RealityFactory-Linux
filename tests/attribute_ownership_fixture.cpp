/* Test-only boot-stage wrapper, linked against the unchanged runtime objects.
 * Common startup/teardown remains real; no level, actor or private data is used. */
#include "RabidFramework.h"
#include <cstdio>
#include <cstring>

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "attribute ownership: FAIL line %d\n", __LINE__); return 1; } } while (0)

static bool list_is(CPersistentAttributes &attributes, const char *const *names, int count)
{
    PersistAttribute *node=attributes.GetAttribute(NULL);
    for (int i=0;i<count;++i) {
        if (!node || std::strcmp(node->Name,names[i])) return false;
        node=attributes.GetAttribute(node);
    }
    return node==NULL;
}

extern "C" int __wrap_RF_RunBootStage(void)
{
    CPersistentAttributes attributes;
    const char *all[]={"head","left","middle","right","tail"};
    const char *after_head[]={"left","middle","right","tail"};
    const char *after_middle[]={"left","right","tail"};
    const char *after_tail[]={"left","right"};
    for (int i=0;i<5;++i) {
        CHECK(attributes.AddAndSet(all[i],10+i)==RGF_SUCCESS);
        CHECK(attributes.AllocateUserData(all[i],32+i)==RGF_SUCCESS);
        std::memset(attributes.UserData(all[i]),i+1,32+i);
    }
    CHECK(list_is(attributes,all,5));
    // Head and non-head deletion must preserve all other nodes and their data.
    CHECK(attributes.RemoveAll("head")==RGF_SUCCESS);
    CHECK(list_is(attributes,after_head,4));
    CHECK(attributes.Value("left")==11 && attributes.UserData("left")[0]==2);
    CHECK(attributes.RemoveAll("middle")==RGF_SUCCESS);
    CHECK(list_is(attributes,after_middle,3));
    CHECK(attributes.Remove("tail")==RGF_SUCCESS);
    CHECK(list_is(attributes,after_tail,2));
    CHECK(attributes.RemoveAll("absent")!=RGF_SUCCESS);
    CHECK(list_is(attributes,after_tail,2));
    // Replacement and explicit deletion each own their new[] byte buffer.
    CHECK(attributes.AllocateUserData("left",80)==RGF_SUCCESS);
    std::memset(attributes.UserData("left"),7,80);
    CHECK(attributes.DeleteUserData("left")==RGF_SUCCESS);
    CHECK(attributes.UserData("left")==NULL);
    CHECK(attributes.GetAttribute(NULL)->UserDataSize==0);
    CHECK(attributes.DeleteUserData("left")==RGF_SUCCESS);
    CHECK(attributes.Clear()==RGF_SUCCESS); // right retains a live byte buffer
    CHECK(attributes.GetAttribute(NULL)==NULL);
    CHECK(attributes.Clear()==RGF_SUCCESS);
    CHECK(attributes.Add("only")==RGF_SUCCESS);
    CHECK(attributes.AllocateUserData("only",19)==RGF_SUCCESS);
    CHECK(attributes.RemoveAll("only")==RGF_SUCCESS);
    CHECK(attributes.GetAttribute(NULL)==NULL);
    CHECK(attributes.Add("destructor")==RGF_SUCCESS);
    CHECK(attributes.AllocateUserData("destructor",23)==RGF_SUCCESS);
    std::fprintf(stderr,"attribute ownership: PASS (head/middle/tail, replacement, explicit delete, clear, final destructor pending)\n");
    return 0; // local destructor plus ordinary common teardown; LSan checks exit
}
