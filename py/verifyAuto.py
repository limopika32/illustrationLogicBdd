from os import listdir,path
from PIL import Image
from pyzbar.pyzbar import decode

dir_path = "qra/"
dlist = [d for d in listdir(dir_path) if path.isdir(path.join(dir_path, d))]

for d in dlist:
    try:
        dfile = [
            f for f in listdir(path.join(dir_path, d)) if path.isfile(path.join(dir_path, d, f)) and f[-4:] == ".png"
        ]
    except FileNotFoundError:
        continue

    last = None
    count = 0
    
    for dat in dfile:
        line = decode(Image.open(path.join(dir_path, d, dat)))
        try:
            curr = line[0].data
        except IndexError as e:
            print("{} at {}".format(e,dat))
            continue
        
        if last != None and last != curr:
            print("Mismatch at {}!  {}, {}".format(dat,last,curr))
            count += 1
        
        last = curr
    
    print("Done! at {} - mismatch: {}/{} - ({})".format(d, count, len(dfile), last))