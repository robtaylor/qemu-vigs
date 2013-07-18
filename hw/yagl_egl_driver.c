#include "yagl_egl_driver.h"

void yagl_egl_driver_init(struct yagl_egl_driver *driver)
{
    driver->dyn_lib = NULL;
}

void yagl_egl_driver_cleanup(struct yagl_egl_driver *driver)
{
    if (driver->dyn_lib) {
        yagl_dyn_lib_destroy(driver->dyn_lib);
    }
}
