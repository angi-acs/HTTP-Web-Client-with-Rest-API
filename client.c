#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>
#include "helpers.h"
#include "requests.h"
#include "parson/parson.h"

int main(int argc, char *argv[]) {
    char *command = calloc(LINELEN, sizeof(char));
    int sockfd;
    char *message;
    char *response;
    char *host = "ec2-34-241-4-235.eu-west-1.compute.amazonaws.com";
    char *content_type = "application/json";
    char *session_cookie = calloc(LINELEN, sizeof(char));
    char *token = calloc(LINELEN, sizeof(char));
    char *url = calloc(LINELEN, sizeof(char));

    char *username = calloc(LINELEN, sizeof(char));
    char *password = calloc(LINELEN, sizeof(char));
    char *title = calloc(LINELEN, sizeof(char));
    char *author = calloc(LINELEN, sizeof(char));
    char *genre = calloc(LINELEN, sizeof(char));
    char *publisher = calloc(LINELEN, sizeof(char));

    bool token_bool = false;
    bool logged_in = false;

    while(1) {
        printf("\nEnter command:\n");
        scanf("%s", command);
        
        if (strcmp(command, "register") == 0) {
            if (logged_in) {
                printf("Please log out first!\n");
            } else {
                sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);

                printf("username = ");
                scanf("%s", username);
                printf("password = ");
                scanf("%s", password);

                JSON_Value *root_value = json_value_init_object();
                JSON_Object *root_object = json_value_get_object(root_value);
                char *serialized_string = NULL;
                json_object_set_string(root_object, "username", username);
                json_object_set_string(root_object, "password", password);
                serialized_string = json_serialize_to_string_pretty(root_value);

                message = compute_post_request(host, "/api/v1/tema/auth/register", content_type, 
                                                &serialized_string, 1, NULL, 0, NULL);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);

                if (strstr(response, "201 Created") != NULL) {
                    printf("Successfully registered user!\n");
                } else if (strstr(response, "400 Bad Request") != NULL) {
                    printf("Error: The username %s is taken!\n", username);
                } else if (strstr(response, "429 Too Many Requests") != NULL) {
                    printf("Too many requests, please try again later.\n");
                }

                json_free_serialized_string(serialized_string);
                json_value_free(root_value);

                close_connection(sockfd);
            }
        }

        if (strcmp(command, "login") == 0) {
            if (logged_in) {
                printf("Please log out first!\n");
            } else {
                sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);

                printf("username = ");
                scanf("%s", username);
                printf("password = ");
                scanf("%s", password);

                JSON_Value *root_value = json_value_init_object();
                JSON_Object *root_object = json_value_get_object(root_value);
                char *serialized_string = NULL;
                json_object_set_string(root_object, "username", username);
                json_object_set_string(root_object, "password", password);
                serialized_string = json_serialize_to_string_pretty(root_value);

                message = compute_post_request(host, "/api/v1/tema/auth/login", content_type, 
                                                &serialized_string, 1, NULL, 0, NULL);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);

                char *cookie;
                cookie = strstr(response, "connect.sid");
                if (cookie != NULL) {
                    session_cookie = strtok(cookie, ";");
                }

                if (strstr(response, "200 OK") != NULL) {
                    printf("User Authenticated Successfully!\n");
                    logged_in = true;
                } else if (strstr(response, "400 Bad Request") != NULL) {
                    printf("Error: Credentials are not good!\n");
                }

                close_connection(sockfd);
                json_free_serialized_string(serialized_string);
                json_value_free(root_value);
            }
        }

        if (strcmp(command, "enter_library") == 0) {
            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);

            message = compute_get_request(host, "/api/v1/tema/library/access", NULL, 
                                        &session_cookie, 1, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);

            char *token_aux;
            token_aux = strstr(response, "token");
            if (token_aux != NULL) {
                token = strtok(token_aux, "\"");
                token = strtok(NULL, "\"");
                token = strtok(NULL, "\"");
            } // extrag din formatul json

            if (strstr(response, "200 OK") != NULL) {
                printf("User Entered Library Successfully!\n");
                token_bool = true;
            } else if (strstr(response, "401 Unauthorized") != NULL) {
                printf("Error: You are not logged in!\n");
            }

            close_connection(sockfd);
        }

        if (strcmp(command, "get_books") == 0) {
            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);

            message = compute_get_request(host, "/api/v1/tema/library/books", NULL, 
                                        NULL, 0, token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);

            char *books;
            books = strstr(response, "[{\"");
            if (books != NULL) {
                puts(books);
            } else if (strstr(response, "500 Internal Server Error") != NULL) {
                printf("Error: You don't have access to the library!\n");
            }

            close_connection(sockfd);
        }

        if (strcmp(command, "get_book") == 0) {
            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);
            
            if (!token_bool) {
                printf("Error: You don't have access to the library!\n");
            } else {
                int id;
                printf("id = ");
                scanf("%d", &id);

                memset(url, LINELEN, 0);
                sprintf(url, "/api/v1/tema/library/books/%d", id);

                message = compute_get_request(host, url, NULL, NULL, 0, token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);

                char *book;
                book = strstr(response, "[{\"");
                if (book != NULL) {
                    puts(book);
                } else if (strstr(response, "No book was found") != NULL) {
                    printf("Error: No book was found!\n");
                }
            }

            close_connection(sockfd);
        }

        if (strcmp(command, "add_book") == 0) {
            if (!token_bool) {
                printf("Error: You don't have access to the library!\n");
            } else {
                sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);

                int page_count;
                printf("title = ");
                scanf(" %[^\n]", title); // citeste pana la new line (regex)
                printf("author = ");
                scanf(" %[^\n]", author);
                printf("genre = ");
                scanf(" %[^\n]", genre);
                printf("page_count = ");
                scanf("%d", &page_count);
                printf("publisher = ");
                scanf(" %[^\n]", publisher);

                JSON_Value *root_value = json_value_init_object();
                JSON_Object *root_object = json_value_get_object(root_value);
                char *serialized_string = NULL;
                json_object_set_string(root_object, "title", title);
                json_object_set_string(root_object, "author", author);
                json_object_set_string(root_object, "genre", genre);
                json_object_set_number(root_object, "page_count", page_count);
                json_object_set_string(root_object, "publisher", publisher);
                serialized_string = json_serialize_to_string_pretty(root_value);

                message = compute_post_request(host, "/api/v1/tema/library/books", content_type, 
                                                &serialized_string, 1, NULL, 0, token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);

                if(strstr(response, "200 OK") != NULL) {
                    puts(serialized_string);
                    printf("Book was added successfully!\n");
                } else {
                    printf("Could not add book!");
                }

                json_free_serialized_string(serialized_string);
                json_value_free(root_value);

                close_connection(sockfd);
            }
        }

        if (strcmp(command, "delete_book") == 0) {
            if (!token_bool) {
                printf("Error: You don't have access to the library!\n");
            } else {
                sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);

                int id;
                printf("id = ");
                scanf("%d", &id);

                memset(url, LINELEN, 0);
                sprintf(url, "/api/v1/tema/library/books/%d", id);

                message = compute_delete_request(host, url, token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);

                if (strstr(response, "200 OK") != NULL) {
                    printf("Book %d was deleted successfully!\n", id);
                } else if (strstr(response, "No book was deleted") != NULL) {
                    printf("Error: No book was found!\n");
                }

                close_connection(sockfd);
            }
        }

        if (strcmp(command, "logout") == 0) {
            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);

            message = compute_get_request(host, "/api/v1/tema/auth/logout", NULL, 
                                        &session_cookie, 1, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);

            logged_in = false;
            token_bool = false;
            memset(session_cookie, LINELEN, 0);
            memset(token, LINELEN, 0);

            if (strstr(response, "You are not logged in!") != NULL) {
                printf("Error: You are not logged in!\n");
            } else if (strstr(response, "200 OK") != NULL) {
                printf ("User %s logged out!\n", username);
            }

            close_connection(sockfd);
        }

        if (strcmp(command, "exit") == 0) {
            break;
        }
    }

    return 0;
}
