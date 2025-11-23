data = open("dht_anomaly_model.tflite", "rb").read()

with open("dht_anomaly_model.cpp", "w") as f:
    f.write('#include "dht_anomaly_model.h"\n\n')
    f.write('const unsigned char dht_anomaly_model_tflite[] = {\n')

    for i, b in enumerate(data):
        if i % 12 == 0:
            f.write("  ")
        f.write(f"0x{b:02x}")
        if i != len(data) - 1:
            f.write(", ")
        if (i + 1) % 12 == 0:
            f.write("\n")

    f.write('\n};\n\n')
    f.write('const unsigned int dht_anomaly_model_tflite_len = sizeof(dht_anomaly_model_tflite);\n')
