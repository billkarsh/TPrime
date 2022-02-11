
Purpose:
+ Map from-stream event times into to-stream times.
Run messages are appended to TPrime.log in the current working directory.

Usage:
>TPrime -syncperiod=1.0 -tostream=path/edgefile.txt <from_data>

From_data:
-fromstream=5,path/edgefile.txt                        ;stream_index,edge_times
-events=5,path/in_eventfile.txt,path/out_eventfile.txt ;stream_index,in_event_times,out_event_times

Notes:
- The sync pulser period can be read from SpikeGLX metadata files 'syncSourcePeriod' (enter value as recorded in file).
- Streams are characterized by a file of pulser edge times.
- Obtain edge files by running CatGT to extract the sync channel from each stream of interest.
- Each from-stream needs a zero-based stream_index that you assign arbitrarily (5 in this example); it merely links event files with their source streams.
- You can list as many -events options as needed, so you can do all translation work in one call.

- Events files can be txt or Kilosort/KS2 npy files.
    + Input and output file types need not match.
    + Times are seconds, ** NOT SAMPLES **.
    + Times are relative to the start of a file.
    + Times must be sorted in ascending order.

- For npy input Events files:
    + We expect an array of doubles with dim {n-values}.

- For txt input Events files:
    + One event time per line.
    + Time must be only item (or first item) on line.
    + Times should be printed to 6 decimal digits precision (30000.000000).

- You can call TPrime from a script.
- You can try it by editing the included 'runit.bat' file. Edit the file to set your own parameters. Then double-click the bat file to run it.
- Options must not have spaces.
- In *.bat files, continue long lines using <space><caret>. Like this ^.
- Remove all white space at line ends, especially after a caret (^).


Change Log
----------
Version 1.7
- Fix an end-of-file parsing bug.

Version 1.6
- Fix potential crash.

Version 1.5
- Improved calling scripts.

Version 1.4
- Working/calling dir can be different from installed dir.
- Log file written to working dir.

Version 1.3
- Handle fortran-order npy files.
- Output npy headers are x64 size.

Version 1.2
- Events files can be txt or npy.

Version 1.1
- Add error messages for file existence.

Version 1.0
- Initial release.


