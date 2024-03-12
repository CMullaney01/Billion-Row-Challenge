# Billion Row Challenge

I am using the setup provided by [dannyvankooten](https://github.com/dannyvankooten/1brc#submitting) to create my billion-row file and analyze it. All my attempts will be my journey learning concurrency.

## The Challenge
[challenge](https://1brc.dev/#the-challenge)
find the min, mean, and max of each weather station!
Format:

```
Hamburg;12.0
Bulawayo;8.9
Palembang;38.8
...
```

### v1:

Single Thread, caluclate min mean max with an array map takes about 5 minutes really not good

### V2:

Trying to have one thread for parsing and one thread for processing with a circular buffer. Actually made it slower!

### V3:
Some fancyness has been added. Reverting back to V1 implementation and making it multithreaded through splitting up the file into chunks and processing them separately. Then combining their outputs for final output. 

I failed to achieve this without memory mapping the data. This is due to small numbers of chunks causing significant overhead in the finding of the chunk start in the file and then running out of memory by reading them line by line. As such memory mapping has been implemented allowing us to iterate through it like an array using off_t (first time using it, very powerful)!

~ 25 seconds

If you run this version we have used chrono to output timestamps, once again the processing of the file is the biggest limiting factor, taking 24.9165 seconds of the total 24.9989 seconds. how the hell do people do this in less than 2 seconds?


