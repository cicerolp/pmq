"""
	A Command-Line Interface to Twitter's REST API and Streaming API.
	-----------------------------------------------------------------
	
	Run this command line script with any Twitter endpoint.  The json-formatted
	response is printed to the console.  The script works with both Streaming API and
	REST API endpoints.

	IMPORTANT: Before using this script, you must enter your Twitter application's OAuth
	credentials in TwitterAPI/credentials.txt.  Log into http://dev.twitter.com to create
	your application.
	
	Examples:

	::
	
		python getStream -endpoint search/tweets -parameters q=zzz
		python getStream -endpoint statuses/filter -parameters track=zzz
		
	These examples print the raw json response with a subset of the json keys 
		
	Documentation for all Twitter endpoints is located at:
		 https://dev.twitter.com/docs/api/1.1
"""

from TwitterAPI import TwitterAPI, __version__
from TwitterAPI.TwitterOAuth import TwitterOAuth
#from __future__ import print_function
import argparse
import codecs
import json
import sys




# print UTF-8 to the console
if sys.platform == "win32":
    from Unicode_win32 import stdout
    sys.stdout = stdout


def _search(name, obj):
    """Breadth-first search for name in the JSON response and return value."""
    q = []
    q.append(obj)
    while q:
        obj = q.pop(0)
        if hasattr(obj, '__iter__') and type(obj) is not str:
            isdict = isinstance(obj, dict)
            if isdict and name in obj:
                return obj[name]
            for k in obj:
                q.append(obj[k] if isdict else k)
    else:
        return None


def _to_dict(param_list):
    """Convert a list of key=value to dict[key]=value"""
    if param_list:
        return {
            name: value for (name, value) in 
                [param.split('=') for param in param_list]}
    else:
        return None


if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='Request any Twitter Streaming or REST API endpoint')
    parser.add_argument(
        '-oauth',
        metavar='FILENAME',
        type=str,
        help='file containing OAuth credentials')
    parser.add_argument(
        '-endpoint',
        metavar='ENDPOINT',
        type=str,
        help='Twitter endpoint',
        required=True)
    parser.add_argument(
        '-parameters',
        metavar='NAME_VALUE',
        type=str,
        help='parameter NAME=VALUE',
        nargs='+')
    parser.add_argument(
        '-fields',
        metavar='NAME',
        type=str,
        help='print a top-level field in the json response',
        nargs='+')
    parser.add_argument(
        '-indent',
        metavar='SPACES',
        type=int,
        help='number of spaces to indent json output',
        default=None)
    args = parser.parse_args()

    #The first level keys to save from json
    keys = ["coordinates","created_at","geo","id","lang","place","text","timestamp_ms"]
    
    try:
        params = _to_dict(args.parameters)
        oauth = TwitterOAuth.read_file(args.oauth)

        api = TwitterAPI(oauth.consumer_key,
                         oauth.consumer_secret,
                         oauth.access_token_key,
                         oauth.access_token_secret)

        response = api.request(args.endpoint, params)

        for item in response.get_iterator():
            js_sub = { key : item[key] if key in item else "none" for key in keys}
            js_sub["hashtags"] = item["entities"]["hashtags"] if "entities" in item else "none"
            json.dump(js_sub,sys.stdout,ensure_ascii=False)
            print("",file=sys.stdout)

    except KeyboardInterrupt:
        print('Terminated by user', file=sys.stderr)
        
    except Exception as e:
        print('STOPPED: %s' % e,file=sys.stderr)
