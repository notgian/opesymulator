# TODO
This file will serve as a simple todo listfor what we still need implemented. Tick things off as you go when you commit your changes, and put your name beside what you did.

- [x] FOR instruction [Gian]
- [ ] The other instructions [Sky]
- [ ] RR Scheduler function
- [ ] Main menu console commands
  - [ ] scheduler-start 
  - [ ] scheduler-stop
  - [ ] report-util
  - [ ] screen (update to match specs** -- a bit of conflicting info about what -s should do, so idk)[^1]
- [x] process screen commands
  - [x] process-smi + changing the behavior of the print command**



[^1]: The `screen -s` command is mentioned multiple times in the specs to do these two functions: to "open" the process screen up or basically attach the process screen (see specs page 3) AND to create a new process (see specs page 2). BOTH follow the exact same format, however another command `screen -r` is mentioned that does the screen attaching. Just in my opinion (Gian's opinion), `screen -s` should probably be used for creating the processes, and `screen -r` for attaching to a process screen.
