#include <pwd.h>
#include <string.h>
#include <syslog.h>

void endpwent()
{
}

static struct passwd pw_user = {
    .pw_name = "user",
    .pw_passwd = "",
    .pw_uid = 1,
    .pw_gid = 1,
    .pw_gecos = "",
    .pw_dir = "/",
    .pw_shell = "/bin/sh"
};

struct passwd * getpwent(void)
{
    syslog(LOG_DEBUG, "libc: getpwent");
    return NULL;
}

struct passwd * getpwnam(const char * name)
{
    if (!strcmp(name, "user"))
        return &pw_user;
    syslog(LOG_DEBUG, "libc: getpwnam: name='%s'", name);
    return NULL;
}

struct passwd * getpwuid(uid_t uid)
{
    if (uid == 1)
        return &pw_user;
    syslog(LOG_DEBUG, "libc: getpwuid");
    return NULL;
}

void setpwent(void)
{
}
