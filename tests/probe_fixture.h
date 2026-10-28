/** @file
 * @brief Expose synthetic call counters for target-window tests, never for production probing.
 */
#ifndef DISKSAN_TEST_PROBE_FIXTURE_H
#define DISKSAN_TEST_PROBE_FIXTURE_H
/** @brief Reset all synthetic observations before an independent fixture invocation. */
void ds_test_probe_reset(void);
/** @brief Return the number of synthetic native provider calls. */
unsigned ds_test_probe_calls(void);
/** @brief Return the number of synthetic selector resolutions. */
unsigned ds_test_probe_resolutions(void);
#endif
