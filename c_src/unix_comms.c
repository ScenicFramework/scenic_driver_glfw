#include "comms.h"

//=============================================================================
// raw comms with host app
// from erl_comm.c
// http://erlang.org/doc/tutorial/c_port.html#id64377
//---------------------------------------------------------
int read_exact(byte* buf, int len)
{
  int i, got = 0;

  do
  {
    if ((i = read(0, buf + got, len - got)) <= 0)
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
      if ((i = write(1, buf + wrote, len - wrote)) <= 0)
      return (i);
      wrote += i;
  } while (wrote < len);

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
  byte buff[4];

  fd_set rfds;
  int    retval;

  // Watch stdin (fd 0) to see when it has input.
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);

  // look for data
  retval = select(1, &rfds, NULL, NULL, ptv);
  if (retval == -1)
  {
    return -1; // error
  }
  else if (retval)
  {
    if (read_exact(buff, 4) != 4)
      return (-1);
    // length from erlang is always big endian
    uint32_t len = *((uint32_t*) &buff);
    swap_little_endian_uint(&len);

    return len;
  }
  else
  {
    // no data within the timeout
    return -1;
  }
}

//---------------------------------------------------------
// return true if the caller side of the stdin pipe has hung up
// http://pubs.opengroup.org/onlinepubs/7908799/xsh/poll.html
// see
// https://stackoverflow.com/questions/25147181/pollhup-vs-pollnval-or-what-is-pollhup
bool isCallerDown()
{
  struct pollfd ufd;
  memset(&ufd, 0, sizeof ufd);
  ufd.fd     = STDIN_FILENO;
  ufd.events = POLLIN;
  if (poll(&ufd, 1, 0) < 0)
    return true;
  return ufd.revents & POLLHUP;
}
