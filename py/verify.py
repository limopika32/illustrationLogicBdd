from os import listdir,path
from PIL import Image
from pyzbar.pyzbar import decode

dir_path = "qrx/ANS/qh09/"

dfile = [ f for f in listdir(dir_path) if path.isfile(path.join(dir_path, f)) and f[-4:] == ".png" ]
# print(dfile)

last = None
total = len(dfile)
count, success, errs = 0, 0, 0

for dat in dfile:
    line = decode(Image.open(dir_path + dat))
    try:
        curr = line[0].data
    except IndexError as e:
        print("{} at {}".format(e,dat))
        errs += 1
        continue

    if last != None and last != curr:
        print("Mismatch at {}!  {}, {}".format(dat,last,curr))
        count += 1
    else:
        success += 1
    
    last = curr

print("Done! - ALL: {}, OK: {}({:7.3f}%), NG: {}({:7.3f}%), ERR: {}({:7.3f}%)".format(
    total, 
    success, success/total*100, 
    count, count/total*100,
    errs, errs/total*100
    )
)