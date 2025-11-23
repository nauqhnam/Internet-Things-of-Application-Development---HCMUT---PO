import numpy as np
import pandas as pd
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

# 1) Đọc dữ liệu từ CSV
df = pd.read_csv("dht_data.csv")   # cùng thư mục với script

# Đảm bảo đúng tên cột:
X = df[["temp", "hum"]].values.astype("float32")
y = df["label"].values.astype("float32")

print("Số mẫu:", X.shape[0])

# 2) (Tuỳ chọn) xáo trộn dữ liệu
idx = np.arange(len(X))
np.random.shuffle(idx)
X = X[idx]
y = y[idx]

# 3) Chia train / val
split = int(0.8 * len(X))
X_train, X_val = X[:split], X[split:]
y_train, y_val = y[:split], y[split:]

print("Train:", X_train.shape[0], "mẫu")
print("Val:", X_val.shape[0], "mẫu")

# 4) Tạo model – input 2 features: temp, hum
model = keras.Sequential([
    layers.Input(shape=(2,)),
    layers.Dense(8, activation="relu"),
    layers.Dense(8, activation="relu"),
    layers.Dense(1, activation="sigmoid")  # output 1 float (0–1)
])

model.compile(
    optimizer="adam",
    loss="binary_crossentropy",
    metrics=["accuracy"]
)

# 5) Train
history = model.fit(
    X_train, y_train,
    epochs=50,
    batch_size=32,
    validation_data=(X_val, y_val)
)

# 6) Đánh giá nhanh
loss, acc = model.evaluate(X_val, y_val, verbose=0)
print(f"Val accuracy: {acc:.4f}")

# 7) Lưu model Keras (tuỳ chọn, để debug trên PC)
model.save("dht_anomaly_model_keras.keras")

# 8) Convert sang TFLite (float input/output, phù hợp ESP32 hiện tại)
converter = tf.lite.TFLiteConverter.from_keras_model(model)
# Tối ưu dung lượng, vẫn dùng float I/O
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

with open("dht_anomaly_model.tflite", "wb") as f:
    f.write(tflite_model)

print("Đã lưu dht_anomaly_model.tflite")
