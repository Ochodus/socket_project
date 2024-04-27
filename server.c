#include "csapp.h"
void op_transaction(int fd);
int read_requesthdrs(rio_t *rp, int log, char *method);
int parse_uri(char *uri, char *filename, char *cgiargs);
void static_serve(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void dynamic_serve(int fd, char *filename, char *cgiargs);
void client_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
	int listenfd, connfd;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	listenfd = open_listenfd(argv[1]);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
		getnameinfo((struct sockaddr*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
		printf("Accepted connection from (%s, %s)\n", hostname, port);
		op_transaction(connfd);
		close(connfd);
	}
}

void op_transaction(int fd) {
	int is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio;
	int log;
	size_t n;

	rio_readinitb(&rio, fd);
	rio_readlineb(&rio, buf, MAXLINE);
	printf("Request headers:\n");
	printf("%s", buf);

	sscanf(buf, "%s %s %s", method, uri, version);
	if (!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0 || strcasecmp(method, "POST") == 0)) {
		client_error(fd, method, "501", "Not Implemented", "Does not implement this method.");
		return;
	}

	int param_len = read_requesthdrs(&rio, log, method);
	rio_readnb(&rio, buf, param_len);

	is_static = parse_uri(uri, filename, cgiargs);
	if (stat(filename, &sbuf) < 0) {
		client_error(fd, filename, "404", "Not found", "Couldn't find this file");
		return;
	}
	if (is_static) {
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			client_error(fd, filename, "403", "Forbidden", "Couldn't read the file");
			return;
		}
		static_serve(fd, filename, sbuf.st_size, method);
	}
	else {
		/* Managing only static server */
	}
}

void client_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
	char buf[MAXLINE];
	sprintf(buf, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n\r\n");
	rio_writen(fd, buf, strlen(buf));

	sprintf(buf, "<html><title>Error</title>");
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "%s: %s\r\n", longmsg, cause);
	rio_writen(fd, buf, strlen(buf));
}

int read_requesthdrs(rio_t *rp, int log, char* method) {
	char buf[MAXLINE];
	int len = 0;
	size_t n;
	do {
		rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
		if (strcasecmp(method, "POST") == 0 && strncasecmp(buf, "Content-Length:", 15) == 0) {
			printf("buf: %s", buf);
			printf("len: %d", len);
			sscanf(buf, "Content-length: %d", &len);
		}
	} while (strcmp(buf, "\r\n"));
	return len;
}

int parse_uri(char *uri, char *filename, char *cgiargs) {
	char *ptr;
	if (!strstr(uri, "cgi-bin")) {
		strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		if (uri[strlen(uri)-1] == '/')
			strcat(filename, "home.html");
	}
	else {
		/* Managing only static server */
		return 0;
	}
}

void get_filetype(char *filename, char *filetype) {
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if (strstr(filename, ".jpg") || strstr(filename, ".jpeg"))
		strcpy(filetype, "image/jpeg");
	else if (strstr(filename, ".mp3"))
		strcpy(filetype, "audio/mp3");
	else if (strstr(filename, ".mp4"))
		strcpy(filetype, "video/mp4");
	else if (strstr(filename, ".pdf"))
		strcpy(filetype, "application/pdf");
	else
		strcpy(filetype, "text/plain");
}

void static_serve(int fd, char *filename, int filesize, char *method) {
	int srcfd;
	char *srcp, filetype[MAXLINE], buf[MAXBUF];

	get_filetype(filename, filetype);
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Web Server\r\n");
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n", filesize);
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
	rio_writen(fd, buf, strlen(buf));
	if (!(strcasecmp(method, "GET"))) {
		srcfd = open(filename, O_RDONLY, 0);
		srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
		close(srcfd);
		rio_writen(fd, srcp, filesize);
		munmap(srcp, filesize);
	}
}

void dynamic_serve(int fd, char *filename, char *cgiargs) {
	/* Managing only static server */
}
