// Host test for mtSliceManager::insertSlicesAtPositions (Lazy Chop batch insert).
// The function body is extracted verbatim from core/source/mtSliceManager.cpp by run_sliceInsert.sh and
// compiled against a minimal strInstrument, so the real source is what is tested.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
const uint8_t MAX_SLICE_NUMBER = 48;
const uint16_t MAX_16BIT = 65535;
struct strInstrument { uint8_t sliceNumber = 0; uint8_t selectedSlice = 0; uint16_t slices[MAX_SLICE_NUMBER] = {}; };
struct mtSliceManager { uint8_t insertSlicesAtPositions(strInstrument*, const uint16_t*, uint8_t); };
#define MOD_LAZY_CHOP 1
#include "extracted_insert.inc"
static int failures = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
int main()
{
    mtSliceManager m; strInstrument i;
    { uint16_t p[] = { 30000, 10000, 20000 }; CHECK(m.insertSlicesAtPositions(&i, p, 3) == 3);
      CHECK(i.sliceNumber == 4 && i.slices[0] == 0 && i.slices[1] == 10000 && i.slices[2] == 20000 && i.slices[3] == 30000);
      CHECK(i.selectedSlice == 3); }                                   // sorted, zero boundary created, last inserted selected
    { uint16_t p[] = { 20000, 15000, 15000 }; CHECK(m.insertSlicesAtPositions(&i, p, 3) == 1);
      CHECK(i.sliceNumber == 5 && i.slices[2] == 15000 && i.slices[3] == 20000 && i.slices[4] == 30000 && i.selectedSlice == 2); } // existing + batch duplicates rejected, existing kept
    { uint16_t p[] = { 15001, 15002 }; CHECK(m.insertSlicesAtPositions(&i, p, 2) == 2); CHECK(i.sliceNumber == 7); }              // closely spaced distinct positions allowed
    { uint16_t p[] = { 65535 }; uint8_t before = i.sliceNumber; CHECK(m.insertSlicesAtPositions(&i, p, 1) == 0 && i.sliceNumber == before); } // sample end rejected
    { strInstrument e; uint16_t p[] = { 5, 5 }; CHECK(m.insertSlicesAtPositions(&e, p, 2) == 1 && e.sliceNumber == 2 && e.slices[0] == 0 && e.slices[1] == 5); }
    { strInstrument e; uint16_t p[] = { 65535 }; CHECK(m.insertSlicesAtPositions(&e, p, 1) == 0 && e.sliceNumber == 0 && e.selectedSlice == 0); } // nothing inserted: untouched, no zero boundary
    { strInstrument e; uint16_t p[60]; for (int k = 0; k < 60; k++) p[k] = 100 + k;
      CHECK(m.insertSlicesAtPositions(&e, p, 60) == 47 && e.sliceNumber == 48); }                          // 48 cap incl. zero boundary
    { strInstrument e; e.sliceNumber = 48; for (int k = 0; k < 48; k++) e.slices[k] = k * 100; uint16_t p[] = { 50 };
      CHECK(m.insertSlicesAtPositions(&e, p, 1) == 0 && e.sliceNumber == 48); }                            // full table untouched
    { strInstrument e; e.sliceNumber = 2; e.slices[0] = 0; e.slices[1] = 400; e.selectedSlice = 1; uint16_t p[] = { 400 };
      CHECK(m.insertSlicesAtPositions(&e, p, 1) == 0 && e.selectedSlice == 1); }                           // all duplicates: selection kept
    printf("%d checks, %d failures\n", checks, failures); return failures != 0;
}
