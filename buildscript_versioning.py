Import("env")
print('##### Starting script. #######')

import datetime
now = datetime.datetime.now()
#
# Build actions only run at buildtime.
#

#def before_elf(source, target, env):
    #print("######   before_elf")
    # do some actions

def after_elf(source, target, env):
    #print("#######   after_elf")
    # run build spiffs.bin
    print('')
    print('#########################################')
    print('##### Starting spiffs.bin update. #######')
    env.Execute("pio run --target buildfs --environment ${PIOENV}")

#def before_buildprog(source, target, env):
    print("######    before_compile")
    # do some actions

def after_buildprog(source, target, env):
    print("#######   after_buildprog")

#def before_upload(source, target, env):
    #print("######   before_upload")
    # do some actions

#def after_upload(source, target, env):
    #print("#######   after_upload")
    # do some actions

#~ env.AddPreAction("$BUILD_DIR/${PROGNAME}.elf", before_elf)
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", after_elf)

#~ env.AddPreAction('buildprog', before_buildprog)
env.AddPostAction('buildprog', after_buildprog)

#~ env.AddPreAction("upload", before_upload)
env.AddPostAction("upload", after_buildprog)

#~ print(env.Dump())
#print('##### End script. #######')
