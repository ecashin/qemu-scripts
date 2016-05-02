/* seekio.c - seek input or output while transferring data
 *            uses 64-bit file offset for large seeks, without
 *	      truncating the output file if it's specified on
 *            the command line.  (Named file must exist.)
 *
 * For a usage summary, do "./seekrw -h".
 *
 * Example where file 20346592-8-8192 contains 8 KiB from position
 * 20346592 KiB, to go into the disk image file, macbook2.img:
 *
 *   o=`ruby -e 'puts ARGV[0].to_i * 1024' 20346592`
 *   oseek=$o nx=8192 seekrw ../../macbook2.img < 20346592-8-8192
 *
 *
 * Environment controls:
 *	iseek	number of bytes to seek in standard input
 *	oseek	number of bytes to seek in standard output
 *	nx	number of bytes to transfer
 *
 * (The dd command doesn't always really seek, but seekrw does.)
 *
 */
#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE		/* for O_DIRECT */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

const char *usage = "usage:\n"
	"\t[nx={transfer}] "
	"[iseek={input seek}] "
	"[oseek={output seek}] "
	"[direct={1 for direct I/O}] "
	"seekrw [output filename]\n"
	"(all numbers are in bytes with 0 default)\n"
	"Standard input and output are used by default.\n"
	"An output file may be named.  It will not be truncated.\n"
	"Negative seek values mean seek backwards from end.\n"
	"The number of bytes to be transfered must be specified for I/O to occur.\n";

void
envchk(off_t *val, const char *nam)
{
	char *p = getenv(nam);
	off_t n;
	char *end;

	if (!p)
		return;
	n = strtoll(p, &end, 0);
	if (errno || end == p) {
		fprintf(stderr,
			"seekrw Error: value (\"%s\") for \"%s\" is invalid\n",
			p, nam);
		if (errno)
			perror("strtoll");
		fputs(usage, stderr);
		exit(EXIT_FAILURE);
	}

	*val = n;
}

off_t xlseek(int fd, off_t offset)
{
	int whence = offset < 0 ? SEEK_END : SEEK_SET;
	off_t res;

	res = lseek(fd, offset, whence);

	if (res == (off_t) -1) {
		perror("lseek");
		exit(EXIT_FAILURE);
	}

	return res;
}

enum { READ, WRITE };

void
io(int fd, char *buf, size_t n, int rw)
{
	char *ionam = rw == READ ? "read" : "write";

	while (n > 0) {
		int x;

		if (rw == READ)
			x = read(fd, buf, n);
		else
			x = write(fd, buf, n);

		if (x == -1) {
			perror(ionam);
			exit(EXIT_FAILURE);
		} else if (x == 0) {
			fprintf(stderr, "seekrw: zero %s\n", ionam);
			exit(EXIT_FAILURE);
		}

		n -= x;
		buf += x;
	}
}

/* nx is in bytes
 */
void xfer(off_t nx, int rdfd, int wrfd)
{
	char *buf;
	size_t n;

	if ((errno = posix_memalign((void **) &buf, 4*1024, 4*1024)) || !buf) {
		perror("posix_memalign");
		exit(EXIT_FAILURE);
	}
	for (; nx > 0; nx -= n) {
		n = nx > sizeof buf ? sizeof buf : nx;

		io(rdfd, buf, n, READ);
		io(wrfd, buf, n, WRITE);
	}
	free(buf);
}

void output_cfg(void)
{
	char *p;
	int fd;

	if (!(p = getenv("of")))
		return;		/* leave stdout alone */
	fd = open(p, O_WRONLY|O_CREAT, 0666);
	if (fd == -1) {
		fprintf(stderr, "seekrw Error: could not open %s: %s\n",
			p, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (dup2(fd, STDOUT_FILENO) == -1) {
		fprintf(stderr, "seekrw Error: could not dup %d: %s\n",
			fd, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[])
{
	off_t iseek, oseek, rd_direct, wr_direct, nx;
	int rdfd = STDIN_FILENO;
	int wrfd = STDOUT_FILENO;

	rd_direct = wr_direct = 0;
	envchk(&rd_direct, "rd_direct");
	envchk(&wr_direct, "wr_direct");
	if (rd_direct)
		rd_direct = O_DIRECT;
	if (wr_direct)
		wr_direct = O_DIRECT;
	fprintf(stderr, "debug: rd_direct(%lu) wr_direct(%lu)\n",
		rd_direct,
		wr_direct);

	if (argc >= 2) {
		if (argc > 3 || !strcmp(argv[1], "-h")) {
			fputs(usage, stderr);
			exit(EXIT_SUCCESS);
		}
		rdfd = open(argv[1], O_RDONLY | (O_DIRECT & rd_direct));
		if (rdfd == -1) {
			perror("open to read");
			exit(EXIT_FAILURE);
		}
	}
	if (argc == 3) {
		wrfd = open(argv[2], O_WRONLY | (O_DIRECT & wr_direct));
		if (wrfd == -1) {
			perror("open to write");
			exit(EXIT_FAILURE);
		}
	}

	iseek = oseek = nx = 0;

	envchk(&iseek, "iseek");
	envchk(&oseek, "oseek");
	envchk(&nx, "nx");
	output_cfg();

	if (iseek)
		xlseek(rdfd, iseek);

	if (oseek)
		xlseek(wrfd, oseek);

	xfer(nx, rdfd, wrfd);

	return 0;
}
