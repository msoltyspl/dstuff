#include "pooler.h"

const char g_xml16tagLE[] = {
  0x3C ,0x00 ,0x3F ,0x00 ,0x78 ,0x00 ,0x6D ,0x00 ,0x6C ,0x00 ,0x20 ,0x00 ,0x76 ,0x00
 ,0x65 ,0x00 ,0x72 ,0x00 ,0x73 ,0x00 ,0x69 ,0x00 ,0x6F ,0x00 ,0x6E ,0x00 ,0x3D ,0x00 ,0x22 ,0x00
 ,0x31 ,0x00 ,0x2E ,0x00 ,0x30 ,0x00 ,0x22 ,0x00 ,0x20 ,0x00 ,0x65 ,0x00 ,0x6E ,0x00 ,0x63 ,0x00
 ,0x6F ,0x00 ,0x64 ,0x00 ,0x69 ,0x00 ,0x6E ,0x00 ,0x67 ,0x00 ,0x3D ,0x00 ,0x22 ,0x00 ,0x55 ,0x00
 ,0x54 ,0x00 ,0x46 ,0x00 ,0x2D ,0x00 ,0x31 ,0x00 ,0x36 ,0x00 ,0x22 ,0x00 ,0x20 ,0x00 ,0x3F ,0x00
 ,0x3E ,0x00 };
const char g_xml16tagBE[] = {
  0x00, 0x3C ,0x00 ,0x3F ,0x00 ,0x78 ,0x00 ,0x6D ,0x00 ,0x6C ,0x00 ,0x20 ,0x00 ,0x76 ,0x00
 ,0x65 ,0x00 ,0x72 ,0x00 ,0x73 ,0x00 ,0x69 ,0x00 ,0x6F ,0x00 ,0x6E ,0x00 ,0x3D ,0x00 ,0x22 ,0x00
 ,0x31 ,0x00 ,0x2E ,0x00 ,0x30 ,0x00 ,0x22 ,0x00 ,0x20 ,0x00 ,0x65 ,0x00 ,0x6E ,0x00 ,0x63 ,0x00
 ,0x6F ,0x00 ,0x64 ,0x00 ,0x69 ,0x00 ,0x6E ,0x00 ,0x67 ,0x00 ,0x3D ,0x00 ,0x22 ,0x00 ,0x55 ,0x00
 ,0x54 ,0x00 ,0x46 ,0x00 ,0x2D ,0x00 ,0x31 ,0x00 ,0x36 ,0x00 ,0x22 ,0x00 ,0x20 ,0x00 ,0x3F ,0x00
 ,0x3E };
const int g_xml16taglen = 80;

const char g_xml8tag[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
const int g_xml8taglen = 39;

const int g_bomLE = 0x0000FEFF;
const int g_bomBE = 0x0000FFFE;

POOLER *g_pool_dynamic = 0;
