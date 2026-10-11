# save as convert_to_c.py

with open("model.tflite", "rb") as f:
    data = f.read()

with open("model.cc", "w") as f:
    f.write("#include <model.h>\n\n")
    f.write("const unsigned char cnn_model[] = {\n")

    for i, byte in enumerate(data):
        if i % 12 == 0:
            f.write("\n ")
        f.write(f"0x{byte:02x}, ")

    f.write("\n};\n")
    f.write(f"const unsigned int cnn_model_len = {len(data)};\n")

print("Done! model_data.h generated.")