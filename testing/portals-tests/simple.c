#include <portals/portals3.h>

int
main(int argc, char **argv)
{
    int rc;
    ptl_handle_ni_t ni_h;

    rc = PtlNIInit(CRAY_QK_NAL, PTL_PID_ANY, NULL, NULL, &ni_h);
    printf("rc = %d\n", rc);

}

