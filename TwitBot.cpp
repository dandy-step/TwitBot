#include "stdafx.h"
#include <math.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include "include\oauthlib.h"
#include "include\twitcurl.h"
#include "include\rapidjson\document.h"
#include "include\rapidjson\writer.h"
#include "include\rapidjson\stringbuffer.h"

#define GEN_RANDOM 0
#define GEN_EXPRESSION 1

bool includeRetweets = true;

struct twitterUser{
	long int id;
	long int statuses_count;
};

struct Expression{
	char* word;
	int strength;
	bool initializedArrays = false;		//flag to know when the arrays below have been set
	Expression* Previous[50];
	int previousStrengths[50];
};

std::string GenerateTweet(Expression* exp, int length, char** dictionary, int dictionarySize, int typeOfGeneration) {
	//todo: take weight into account

	//put random words in, no logic
	std::string tweet;
	std::string lastTweet;
	bool charLimit = false;
	unsigned int tweetLength = 0;

	//roll desired tweet length
	int lengthTarget = 140 - (rand() % 120);

	switch (typeOfGeneration) {
	case GEN_RANDOM:
		while (!charLimit) {
			if (tweetLength > 0) {
				tweet.append(" ");
				lastTweet = tweet;
			}
			int roll = rand() % length;
			tweet.append(exp[roll].word);
			tweetLength = strlen(tweet.c_str());
			if (tweetLength >= lengthTarget) {
				tweet = lastTweet;
				charLimit = true;
			}
		}
		break;

	case GEN_EXPRESSION:
		while (!charLimit) {
			if (tweetLength > 0) {
				tweet.append(" ");
				lastTweet = tweet;
			}

			//roll on whether to include expression now
			int roll = rand() % 100;
			printf("Roll: %d\n", roll);
			if (roll > 85) {
				//roll on expression to include
				roll = rand() % dictionarySize;
				tweet.append(dictionary[roll]);
				printf("Using expression \"%s\" during generation.\n", dictionary[roll]);
			}
			else {
				tweet.append(exp[rand() % length].word);
			}

			tweetLength = strlen(tweet.c_str());
			if (tweetLength >= lengthTarget) {
				tweet = lastTweet;
				charLimit = true;
			}
		}
		break;

	default:
		printf("No valid generation type provided!\n");
		tweet[0] = NULL;
	}

	//clip end of tweet to overwrite space that gets added on the loop
	if (tweet.length() > 0) {
		tweet[tweet.length() - 1] = '\0';
	}
	return tweet;
}

int _tmain(int argc, _TCHAR* argv[])
{
	bool result;
	twitCurl twit;
	std::string accessTokenKey;
	std::string accessTokenSecret;
	FILE* tokenKey;
	FILE* tokenSecret;

	//source files
	FILE *twitterUsername, *twitterPassword, *consumerKey, *consumerSecret;
	char value[2048];

	fopen_s(&twitterUsername, "username.txt", "r");
	if (twitterUsername) {
		fscanf_s(twitterUsername, "%s", value, sizeof(value));
		twit.setTwitterUsername(value);
		fclose(twitterUsername);
	} else { printf("Failed to open username.txt!\n"); return 1; }

	fopen_s(&twitterPassword, "password.txt", "r");
	if (twitterPassword) {
		fscanf_s(twitterPassword, "%s", value, sizeof(value));
		twit.setTwitterPassword(value);
		fclose(twitterPassword);
	} else { printf("Failed to open password.txt!\n"); return 1; }

	fopen_s(&consumerKey, "consumerkey.txt", "r");
	if (consumerKey) {
		fscanf_s(consumerKey, "%s", value, sizeof(value));
		twit.getOAuth().setConsumerKey(value);
		fclose(consumerKey);
	} else { printf("Failed to open consumerkey.txt!\n"); return 1; }

	fopen_s(&consumerSecret, "consumersecret.txt", "r");
	if (consumerSecret) {
		fscanf_s(consumerSecret, "%s", value, sizeof(value));
		twit.getOAuth().setConsumerSecret(value);
		fclose(consumerSecret);
	} else { printf("Failed to open consumersecret.txt!\n"); return 1; }

	//Check if there is saved tokenKey and tokenSecret from previous run
	fopen_s(&tokenKey, "tokenkey", "r");
	fopen_s(&tokenSecret, "tokenSecret", "r");

	if ((tokenKey && tokenSecret)) {
		char key[512];
		char secret[512];
		fgets(key, 512, tokenKey);
		fgets(secret, 512, tokenSecret);
		twit.getOAuth().setOAuthTokenKey((std::string)key);
		twit.getOAuth().setOAuthTokenSecret((std::string)secret);
	}
	else {
		std::string authURL;
		twit.oAuthRequestToken(authURL);
		result = twit.oAuthHandlePIN(authURL);
		result = twit.oAuthAccessToken();
		twit.getOAuth().getOAuthTokenKey(accessTokenKey);
		twit.getOAuth().getOAuthTokenSecret(accessTokenSecret);

		//write tokenKey and tokenSecret we got so we can reuse it the next time
		if (accessTokenKey.length() > 0) {
			fopen_s(&tokenKey, "tokenKey", "w+");
			fputs(accessTokenKey.data(), tokenKey);
			fclose(tokenKey);
		}

		if (accessTokenSecret.length() > 0) {
			fopen_s(&tokenSecret, "tokenSecret", "w+");
			fputs(accessTokenSecret.data(), tokenSecret);
			fclose(tokenSecret);
		}
	}

	std::string response;
	twit.accountVerifyCredGet();
	twit.getLastWebResponse(response);

	//I don't think search is even working with this library right now, getting error even in the example code
	std::string search = "https://api.twitter.com/1.1/search/tweets.json?q=twitterapi";
	twit.search(search);
	twit.getLastWebResponse(response);
	//printf("%s\n", response.data());

	FILE *targetAccount;
	std::string userProfile;

	//open text file with account to target
	fopen_s(&targetAccount, "source_twitter_account.txt", "r");
	if (targetAccount) {
		fscanf_s(targetAccount, "%s", value, sizeof(value));
		twit.userGet(value);
		fclose(targetAccount);
	} else {
		printf("Failed to open source_twitter_account.txt", "r"); return 1; }

	twit.getLastWebResponse(response);
	FILE* content;
	fopen_s(&content, "content.json", "w+");
	fwrite(response.data(), response.length(), 1, content);
	fclose(content);

	rapidjson::Document d;
	d.Parse(response.c_str());
	printf("This is the value: %s\n", d["profile_location"]["id"].GetString());

	const int tweetNum = 200;
	const char* tweetGroup[tweetNum];
	int counter = 0;
	const char* user = d["id_str"].GetString();
	FILE* jsonResponse;
	for (int i = 0; counter < tweetNum - 1; i++) {
		if (i == 0) {
			twit.timelineUserGet(true, true, 200, user, true);
		}

		twit.getLastWebResponse(response);
		fopen_s(&jsonResponse, "jsonResponse", "wb+");
		int test = response.length();
		size_t mega = fwrite(response.data(), test, 1, jsonResponse);
		char err[2048];
		printf("%s\n", strerror_s(err, 2048, ferror(jsonResponse)));
		fclose(jsonResponse);
		d.Parse(response.c_str());
		for (int x = 0; x < d.Size(); x++) {
			tweetGroup[counter] = d[x]["text"].GetString();
			if ((tweetGroup[counter][0] == 'R') && (tweetGroup[counter][1] == 'T')) {
				printf("Detected retweet\n");
				if (includeRetweets == true) {
					tweetGroup[counter] = d[x]["retweeted_status"]["text"].GetString();
				}
				else {
					tweetGroup[counter] = "";
				}
				
			}
			counter++;
		}
	}
	for (int i = 0; i < tweetNum - 1; i++) {
		printf("Tweet #%d - %s\n", i, d[i]["text"].GetString());
	}

	char* context = NULL;
	char* token;
	int expression = 0;
	char RTnonNull[] = { 'R', 'T' };
	Expression exps[256 * 7];
	char* tokenCollection[140];
	bool repeatWord = false;
	int tokenCounter = 0;
	int tokenStreak = 0;
	for (int i = 0; i < counter; i++) {
		tokenCounter = 0;
		memset(tokenCollection, NULL, sizeof(char*) * 140);
		while (token = strtok_s((char *)tweetGroup[i], " ", &context)) {
			repeatWord = false;

			//ignore tokens
			if ((!memcmp(token, "RT", 2)) || (!memcmp(token, "http", 4)) || (strchr(token, '@')) || (strchr(token, '\n'))) {
				tweetGroup[i] = context;
				continue;
			}

			//check for garbage tokens
			int strLen = strlen(token);
			for (int x = 0; x < strLen; x++) {
				if (token[x] < 0 && x >= 3) {
					printf("Malformed token %s, clipping | ", token);
					token[x] = '\0';
					printf("New token: %s\n", token);
					break;
				}
				else if (token[x] < 0 && x < 3) {
					printf("Detected short malformed token! - %s in tweet #%d\n", token, i);
					token = NULL;
					break;
				}
			}

			if (token == NULL) {
				tweetGroup[i] = context;
				continue;
			}

			tokenCollection[tokenCounter] = token;

			for (int x = 0; x < expression; x++) {
				if (!strcmp(token, exps[x].word)) {
					exps[x].strength++;
					repeatWord = true;

					//attempt at implementing recognition of expressions that are composed of more than one word
					if (repeatWord && tokenCounter > 2) {
						//set memory to zero
						if (exps[x].initializedArrays == false) {
							memset(exps[x].Previous, NULL, sizeof(Expression*) * 50);
							memset(exps[x].previousStrengths, NULL, sizeof(int) * 50);
							exps[x].initializedArrays = true;
						}

						//record all expressions so far in an array for every single tweet
						//when a repeat word is found, store the word that came immediately before as an expression in the Previous pointer array
						//if value was already there, increase strength
						//if new, record it
						for (int y = 0; y < expression; y++) {
							if (!strcmp(exps[y].word, tokenCollection[tokenCounter - 1])) {
								printf("Found neighbour \"%s\" for \"%s\"", exps[y].word, exps[x].word);

								//find empty array member to place
								int currentMember = 0;
								bool alreadyThere = false;
								while (exps[x].Previous[currentMember] != NULL) {
									if (exps[x].Previous[currentMember] == &exps[y]) {
										exps[x].previousStrengths[currentMember] += 1;
										printf(" | Found repeat, strength %d, position %d |\n", exps[x].previousStrengths[currentMember], currentMember);
										alreadyThere = true;
										break;
									}
									currentMember++;
									if (currentMember >= 100) {
										printf(" Going out of bounds!");
									}
								}

								//place it
								if (!alreadyThere) {
									printf(" Found open slot at position %d!\n", currentMember);
									exps[x].Previous[currentMember] = &exps[y];
									exps[x].previousStrengths[currentMember] = 1;
								}

								break;
							}
						}

						break;
					}
				}
			}

			tokenCounter++;

			if (repeatWord) {
				tweetGroup[i] = context;
				continue;
			}
			else {
				exps[expression].word = token;
				exps[expression].strength = 0;
				expression++;
				tweetGroup[i] = context;
			}
		}
	}

	//print commonly used expressions and allocate a char* array to hold the most popular ones
	//todo: handle ties
	char** commons = (char**)malloc(sizeof(char*) * expression);
	for (int i = 0; i < expression; i++) {
		commons[i] = (char*)malloc(sizeof(char) * 30);
		memset(commons[i], NULL, sizeof(char) * 30);
	}

	int commonCounter = 0;
	printf("Expressions!\n");
	for (int i = 0; i < expression; i++) {
		int maxStrength = 0;
		Expression* maxExp = NULL;
		if (exps[i].initializedArrays == true) {
			int current = 0;
			while (exps[i].Previous[current] != NULL) {
				printf("\"%s %s\", str: %d | ", exps[i].Previous[current]->word, exps[i].word, exps[i].previousStrengths[current]);
				if (exps[i].previousStrengths[current] > maxStrength) {
					maxStrength = exps[i].previousStrengths[current];
					maxExp = exps[i].Previous[current];
				}
				current++;
			}
			printf("\nMost powerful for %s: \"%s %s\" (str: %d)\n\n", exps[i].word, maxExp->word, exps[i].word, maxStrength);

			strcpy_s(commons[commonCounter], 30, maxExp->word);
			strcat_s(commons[commonCounter], 30, " ");
			strcat_s(commons[commonCounter], 30, exps[i].word);
			commonCounter++;
		}
	}

	printf("Most popular expressions: \n");
	for (int i = 0; i < commonCounter; i++) {
		printf("%s\n", commons[i]);
	}

	std::string status;
	char answer = NULL;
	while (answer != 'y' && answer != 'Y') {
		status = GenerateTweet(exps, expression - 1, commons, commonCounter, GEN_EXPRESSION);
		printf("Generated tweet: \"%s\"\n Do you want to tweet it? (Y/N)\n", status.c_str());
		scanf_s(" %1c", &answer);
	}

	twit.statusUpdate(status);
	printf("Status posted! Press Enter to exit.\n");
	char test;
	scanf_s("%c", &test);	//eat last character that remains in stdin after you press enter (no good way to clear stdin that is portable)
	while ((answer = getchar()) != '\n') {

	}

	return 0;
}

