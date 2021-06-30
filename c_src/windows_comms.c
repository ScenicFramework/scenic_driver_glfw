#include "comms.h"

//---------------------------------------------------------
int read_exact(byte* buf, int len)
{
  int i, got = 0;

  do
  {
    if ((i = fread(buf + got, sizeof(byte), len - got, stdin)) <= 0)
      return (i);
    got += i;
  } while (got < len);

  return (len);
}

//---------------------------------------------------------
int write_exact(byte* buf, int len)
{
  int i, wrote = 0;

  do
  {
    if ((i = fwrite(buf + wrote, sizeof(byte), len - wrote, stdout)) <= 0)
      return (i);
    wrote += i;
  } while (wrote < len);

  fflush(stdout);
  return (len);
}

//---------------------------------------------------------
// Starts by using select to see if there is any data to be read
// if not in timeout, then returns with -1
// Setting the timeout too high means input will be laggy as you
// are starving the input polling. Setting it too low means using
// energy for no purpose. Probably best if set similar to the
// frame rate
int read_msg_length(struct timeval * ptv)
{
  DWORD bytesAvailable = 0;
  HANDLE h_stdin = GetStdHandle(STD_INPUT_HANDLE);

  if (PeekNamedPipe(h_stdin, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable) {
      byte buff[4];

      if (read_exact(buff, 4) != 4)
      return (-1);

      // length from erlang is always big endian
      uint32_t len = *((uint32_t*) &buff);

    return ntoh_ui32(len);
  }

  return -1;
}

//---------------------------------------------------------
// return true if the caller side of the stdin pipe has hung up
bool isCallerDown()
{
  DWORD bytesAvailable = 0;
  return !PeekNamedPipe(GetStdHandle(STD_INPUT_HANDLE), NULL, 0, NULL, &bytesAvailable, NULL);
}

//---------------------------------------------------------
// gettimeofday implementation sourced from https://stackoverflow.com/a/26085827
int gettimeofday(struct timeval * tp, struct timezone * tzp) {
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}
