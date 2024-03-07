# Notes on Pipes and Fifo:

## Pipes:
```c
/*
Pipes and FIFOs (also known as named pipes) provide a unidirectional 
interprocess communication channel. A pipe has a read-end and a write-end. 
Data written to the write-end of a pipe can be read from the read-end of the pipe.

The only difference between pipes and FIFOs is the manner in which they are 
created and opened. Once these tasks have been accomplished, I/O on pipes 
and FIFOs has exactly the same semantics.

If a process attempts to read from an empty pipe, then read() will block 
until data is available. If a process attempts to write to a full pipe, then write() 
blocks until sufficient data has been read from the pipe to allow the write to complete.

The communication channel provided by a pipe is a byte stream: 
there is no concept of message boundaries.

If all file descriptors referring to the write end of a pipe have been closed, 
then an attempt to read() from the pipe will see end-of-file (read() will return 0). 
If all file descriptors referring to the read end of a pipe have been closed, 
then a write() will cause a SIGPIPE signal to be generated for the calling process.

A pipe has a limited capacity. If the pipe is full, then a write() will block or fail,
depending on whether the O_NONBLOCK flag is set.

POSIX.1 says that writes of less than PIPE_BUF bytes must be atomic: 
the output data is written to the pipe as a contiguous sequence.
Writes of more than PIPE_BUF bytes may be nonatomic: 
the kernel may interleave the data with data written by other processes.

On Linux PIPE_BUF = 4kb and according to POSIX PIPE_BUF >= 512b.
 
O_NONBLOCK disabled, n <= PIPE_BUF
      All n bytes are written atomically; write() may block if
      there is not room for n bytes to be written immediately

O_NONBLOCK enabled, n <= PIPE_BUF
      If there is room to write n bytes to the pipe, then
      write() succeeds immediately, writing all n bytes;
      otherwise write() fails, with errno set to EAGAIN.

O_NONBLOCK disabled, n > PIPE_BUF
      The write is nonatomic: the data given to write() may be
      interleaved with write()s by other process; the write()
      blocks until n bytes have been written.

O_NONBLOCK enabled, n > PIPE_BUF
      If the pipe is full, then write() fails, with errno set
      to EAGAIN. Otherwise, from 1 to n bytes may be written
      (i.e., a "partial write" may occur; the caller should
      check the return value from write() to see how many bytes
      were actually written), and these bytes may be interleaved
      with writes by other processes.
*/

// Aby ustawić łącze w tryb O_NONBLOCK korzystamy z następującej funkcji:
int set_nonblock(int desc) {
    int oldflags = fcntl(desc, F_GETFL, 0);
    if (oldflags == -1)
        return -1;
    oldflags |= O_NONBLOCK;
    return fcntl(desc, F_SETFL, oldflags);
}

#include <unistd.h>
int pipe(int fildes[2]);
/* Opis:
    - creates an interprocess communication channel.
    - places two file descriptors into fildes[0] and fildes[1], that refer to the read and write ends of the pipe.
    - data can be written to the file descriptor fildes[1] and read from file descriptor fildes[0].
    - data is accessed on a first-in-first-out basis.
    - on success returns 0, otherwise -1 (and errno is set to indicate the error).
    - a common practice for the process is to close the unused end of the inherited pipe.
*/

FILE* fdopen(int fd, const char* mode);
/* Opis:
    - requires: _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE.
    - wraps a raw file descriptor into a standard C stream object.
    - mode follows the same rules as for fopen function.
    - we use "r" for fd[0] and "w" for fd[1].
    - on failure returns NULL.
*/

#include <stdio.h>
int fileno(FILE *stream);
/* Opis:
    - maps a stream pointer to a file descriptor.
*/
#include <stdio.h>
int setvbuf(FILE* stream, char* buf, int type, size_t size);
/* Opis:
    - assigns buffering to a stream.
    - it may be used after the stream pointed to by stream is associated with an open file but before any other operation.
    - _IOFBF causes input/output to be fully buffered.
    - _IOLBF causes input/output to be line buffered.
    - _IONBF causes input/output to be unbuffered.
    - if buf is not a null pointer, the array it points to may be used instead of a buffer allocated by setvbuf() and the argument size specifies the size of the array.
    - on success returns 0, otherwise -1 (and errno is set to indicate the error).
*/

ssize_t write(int fildes, const void *buf, size_t nbyte);
/* Opis:
    - attempts to write nbytes from the buffer pointed to by buf to the file associated with the open file descriptor fildes.
    - if interrupted by a signal before it writes any data, it returns -1 and sets EINTR.
    - if interrupted by a signal after it writes some data, it returns the number of bytes written.
    - on failure returns -1 and sets errno to indicate the error.
    
Write requests to a pipe or FIFO are handled in the same way as a regular file with the following exceptions:
- There is no file offset associated with a pipe, hence each write request shall append to the end of the pipe.
- Write requests of {PIPE_BUF} bytes or less shall not be interleaved with data from other processes doing writes on the same pipe. Writes of greater than {PIPE_BUF} bytes may have data interleaved, on arbitrary boundaries, with writes by other processes, whether or not the O_NONBLOCK flag of the file status flags is set.
- If the O_NONBLOCK flag is clear, a write request may cause
the thread to block, but on normal completion it shall return nbyte.
*/

ssize_t read(int fildes, void *buf, size_t nbyte);
/* Opis:
    - attemts to read from a file given by fildes file descriptor.
    - if interrupted by a signal before it reads any data, it shall return -1 with errno set to [EINTR].
    - if a read() interrupted by a signal after it has successfully read some data, it shall return the number of bytes read.
    - if the write-end of pipe was closed, read() returns 0.
    
When attempting to read from an empty pipe or FIFO:
- If no process has the pipe open for writing, read() shall return 0 to indicate end-of-file.
- If some process has the pipe open for writing and O_NONBLOCK is set, read() shall return -1 and set errno to [EAGAIN].
- If some process has the pipe open for writing and O_NONBLOCK is clear, read() shall block the calling thread until some data is written or the pipe is closed by all processes that had the pipe open for writing.
*/

/*
Important errors and signals:
- ESPIPE - the file is incapable of seeking.
- EPIPE - an attempt is made to write to a pipe or FIFO that is not
open for reading by any process, or that only has one end
open. A SIGPIPE signal shall also be sent to the thread.
- SIGPIPE - if read-end of the pipe is closed and you try to write to the pipe, system sends SIGPIPE signal to the process and write() returns EPIPE error (you can handle the signal by setting a signal handler for SIGPIPE).
*/
```

## Fifo:
```c
/*
A FIFO special file (a named pipe) is similar to a pipe, except
that it is accessed as part of the filesystem. It can be opened
by multiple processes for reading or writing.

The FIFO must be opened on both ends (reading and writing) before 
data can be passed. Normally, opening the FIFO blocks until the other
end is opened also.

A process can open a FIFO in nonblocking mode. In this case,
opening for read-only succeeds even if no one has opened on the
write side yet and opening for write-only fails with ENXIO (no
such device or address) unless the other end has already been
opened.
*/
#include <unistd.h>
int unlink(const char *path);
/* Opis:
    - removes a link to a file.
    - on success returns 0, otherwise -1 and sets errno to indicate the error.
*/

#include <sys/stat.h>
int mkfifo(const char *path, mode_t mode);
/* Opis:
    - makes a FIFO special file.
*/
```
