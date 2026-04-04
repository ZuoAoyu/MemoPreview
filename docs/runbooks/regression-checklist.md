# Regression Checklist

## Scope
- SuperMemo window discovery and content extraction
- `main.tex` incremental update and debounce behavior
- latex compile start/stop and crash auto-restart behavior

## Functional Checks
1. Start app and verify SuperMemo windows are listed after clicking `刷新`.
2. Switch between two SuperMemo items and verify tab content updates within 1 second.
3. Edit content in SuperMemo rapidly and verify `main.tex` updates without continuous high-frequency writes.
4. Switch template and verify `main.tex` reflects the selected template immediately.
5. Click `开始` with valid settings and verify compile status transitions to running.
6. Click `结束` and verify compile status transitions to stopped and buttons reset.

## Failure-Recovery Checks
1. Configure an invalid `latexmk` path and verify `开始` reports startup failure.
2. Restore valid `latexmk` path and verify compile can start again.
3. Force-kill latex child process and verify auto-restart logs appear with retry delay.
4. Force repeated restart failures and verify retry delay increases up to cap.

## Data Integrity Checks
1. Verify generated `main.tex` contains all IE control sections in expected order.
2. Verify `%CONTENT%` placeholder is replaced and no raw placeholder remains.
3. Verify log window still receives compile logs after restart.
