import web_fun

#%%
try:
    jdata_get = web_fun.get_operation_status()
    timetoTrig = jdata_get["timetotrig"]
except Exception as e:
    print(str(e))
    print("failed to get status")

#%%
if timetoTrig != "--:--":
    t = 60*int(timetoTrig.split(':')[0])+int(timetoTrig.split(':')[1])
    print("time to start (seconds): ", t)
else:
    print("discharge not start")
