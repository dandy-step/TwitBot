# TwitBot

An ebooks-style Twitter bot that generates tweets based on the last 200 tweets of a certain user. It uses twitcurl to manage the Twitter API and rapidjson to parse the .json response.

How to use:
* Get your consumer key and secret by registering as a developer through Twitter
* Place consumerkey.txt, consumersecret.txt, username.txt and password.txt files in the root directory (your consumer key, consumer secret, account which will be tweeting, and password for that account. Just place the text, no formatting necessary)
* Place source_twitter_account.txt in the root directory (the account your wish to generate tweets from)