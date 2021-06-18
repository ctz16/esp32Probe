import socketio
import json
import urllib.request
import sys


def on_socket_response(*args, trig_event, mds_event, is_autorun, is_savedata):
    # print('on_socket_response ')
    jdata = args[0]
    #print('on_socket_response:{}'.format(str(jdata)))
    # print(jdata)
    if 'autorun' in jdata:
        is_autorun.value = bool(int(jdata['autorun']))
    if 'savedata' in jdata:
        is_savedata.value = bool(int(jdata['savedata']))
    if 'trigger' in jdata:
        if int(jdata['trigger']):
            if is_savedata.value:
                mds_event.set()
            trig_event.set()
        else:
            mds_event.clear()
            trig_event.clear()       
        # auto run

def get_operation_status():
    url_get = 'http://192.168.1.52/operation'
    jdata_get={}
    try:
        data = urllib.request.urlopen(url_get, timeout=10).read()
        jdata_get = json.loads(str(data, 'utf-8'))['operation'][0]
    except urllib.error.URLError as e:
        print(time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(time.time())))
        print('Connection Failure')
        sys.stdout.flush()
    except ConnectionResetError as e:
        print(time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(time.time())))
        print('ConnectionResetError')
        sys.stdout.flush()
    except Exception as e:
        print(time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(time.time())))
        print('Unexpected Error'+ str(e))
        sys.stdout.flush()
    finally:
        return jdata_get


def post_operation_status(json_dict):
    url_post = 'http://192.168.1.52/operation'
    json2bytes = json.dumps(json_dict).encode('utf-8')
    req = urllib.request.Request(url_post)
    req.add_header('Content-Type', 'application/json; charset=utf-8')
    req.add_header('Content-Length', len(json2bytes))
    try:
        response = urllib.request.urlopen(req, json2bytes,timeout=2).read()
    except Exception as e:
        print('ERROR:post_operation_status:'+str(e))
        sys.stdout.flush()
        return False
    else:
        return True
        

