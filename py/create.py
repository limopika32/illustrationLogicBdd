import qrcode
import numpy
from PIL import Image

def makeQR(data,ver=1,etype=qrcode.ERROR_CORRECT_L,box=1,border=0):
    qr = qrcode.QRCode( # type: ignore
        version = ver,
        error_correction = etype,
        box_size = box,
        border = border
    )
    qr.add_data(data)
    qr.make()
    return qr.make_image()

def makeProb(array):
    max_hint=0
    probC = list()
    probR = list()
    # col >---
    # row vvvv
    for i in range(len(array)):
        col = list(); row = list()
        numC = 0; numR = 0
        for j in range(len(array[i])):
            if not array[i][j]:
                # when marked
                numC += 1
            else:
                if numC != 0: 
                    col.append(numC)
                    numC = 0
            if not array[j][i]:
                # when marked
                numR += 1
            else:
                if numR != 0 :
                    row.append(numR)
                    numR = 0
        if numC!=0 or len(col)==0 : col.append(numC)
        probC.append(col)
        if numR!=0 or len(row)==0 : row.append(numR)
        probR.append(row)

        max_hint = max(max_hint,len(col),len(row))

    return (probC,probR,max_hint)

txt = "c"

makeQR(txt).save("./temp2.png")

im = numpy.array(Image.open("./temp2.png"))

print("image...")
p_count = 0
for i in range(len(im)):
    tmp = ""
    for j in range(len(im[i])):
        tmp += "  " if im[i][j] else "##"
        if not im[i][j] : p_count+=1
    print(tmp)

print("making prob...".rstrip("\n"))
q = makeProb(im)
print("done \tsize: {} x {}".format(len(q[0]),len(q[1])))
print()

with open("hintN.txt",mode="w",encoding="utf-8",newline="\n") as f:
    f.write("{},{},{}".format(len(q[0]), len(q[1]), q[2]))
    f.write("\n")
    for i in q[0]:
        f.write(str(i).strip("[]"))
        f.write("\n")
    for i in q[1]:
        f.write(str(i).strip("[]"))
        f.write("\n")

with open("hintN.txt",mode="r",encoding="utf-8") as f:
    ts = f.read()

print(ts)