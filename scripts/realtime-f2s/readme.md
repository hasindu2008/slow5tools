# Real-time FAST5 to SLOW5 conversion

This can be used on your computer where you are doing the sequencing acquisition, for instance, the laptop connected to the MinION and running MinKNOW.

## Pre-requisites
- inotify-tools:  On Ubuntu `sudo apt install inotify-tools`
- slow5tools in the path or SLOW5TOOLS environment variable set to the slow5tools location.

## Limitations
- Only tested for Linux at the moment (does not work on mounted directories in WSL due to its [inotify limitations](https://github.com/microsoft/WSL/issues/4739)).


## Real run

Assume your sequencing data directory is */data* and you are sequencing an experiment called *my_sequencing_experiment* on to */data/my_sequencing_experiment*. Simply run the following for real-time FAST5 to SLOW5 conversion.

```
./realf2s.sh -m /data/my_sequencing_experiment
```

This script will monitor the specified directory */data/my_sequencing_experiment* and as soon as a newly generated *FAST5* is found, it will convert it. The converted slow5 files will be under */data/my_sequencing_experiment/parent_dir_of_fast5/slow5* and the individual logs from each slow5 conversion will be under */data/my_sequencing_experiment/parent_dir_of_fast5/slow5_logs*.

Brief log messages (including any conversion failures) are written to the terminal as well as */data/my_sequencing_experiment/realtime_f2s.log*. The list of files that were detected by the monitor and which the conversion was attempted will be written to */data/my_sequencing_experiment/realtime_f2s_attempted_list.log*. If any conversions failed, the names of the *FAST5* files will be written to *realtime_f2s_failed_list.log*. In addition, there will be some other debug/trace logs (e.g.,*realtime_f2s_monitor_trace.log*).

The monitoring script will terminate if it idles for 3 hours, i.e., no new FAST5 files were created under */data/my_sequencing_experiment/*, the script will terminate assuming that the sequencing run has completed. Just before termination, the script will check for any left over FAST5 and will convert them if present. Also, it will do a brief check on the file count and print some statistics any warnings if any.

### Options

* -m STR
    The sequencing experiment directory to be monitored. This is usually where MinKNOW writes data for your experiment e.g., */data/my_sequencing_experiment/* or */var/lib/minknow/data/my_sequencing_experiment/*.
* -r
    Resumes a previous live conversion. This option is useful if the real-time conversion abruptly stopped in the middle and you now want to resume the live conversion.
* -t INT
    Timeout in seconds [default: 10800]. The script will end if no new FAST5 were written for this specified period of time.


## Simulation

Say you have some FAST5 files in a directory at */data2/previous_run*. You can test our real-time conversion script (*realf2s.sh*) by simulating a run based on these existing FAST5 files (*monitor/simulator.sh*).

First create a directory to represent our simulated sequencing run, for instance `mkdir /data/my_simulated_run`.
Now launch the real-time conversion script to monitor this directory for newly created FAST5.

```
./realf2s.sh -m /data/my_simulated_run
```

Take another terminal and launch the simulator now.
```
monitor/simulator.sh /data2/previous_run /data/my_simulated_run
```

*monitor/simulator.sh* will copy FAST5 files from */data2/previous_run* to */data/my_simulated_run/fast5*, one at a time at a default interval of 10 seconds. *realf2s.sh* will monitor the */data/my_simulated_run/* and will convert newly created fast5 files. You should see the converted slow5 file under */data/my_simulated_run/slow5*.
