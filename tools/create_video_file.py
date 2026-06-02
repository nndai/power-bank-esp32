import cv2
import struct

def video_to_container(video_path, output_path):
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        raise RuntimeError("Không mở được video")

    fps = int(cap.get(cv2.CAP_PROP_FPS))  # Lấy FPS
    frame_count = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

    with open(output_path, "wb") as f:
        # Header: magic + frameCount + fps
        f.write(b"\xFE\xFA")
        f.write(struct.pack("<I", frame_count))  # u32 little endian
        f.write(struct.pack("<H", fps))          # u16 little endian

        # Lặp qua từng frame
        idx = 0
        while True:
            ret, frame = cap.read()
            if not ret:
                break

            # Mã hóa frame thành JPEG
            success, encoded = cv2.imencode(".jpg", frame, [int(cv2.IMWRITE_JPEG_QUALITY), 85])
            if not success:
                continue

            data = encoded.tobytes()
            f.write(struct.pack("<I", len(data)))  # độ dài u32 LE
            f.write(data)

            idx += 1
            # if idx == frame_count - 3:
            #     break

    cap.release()
    print(f"Done. Saved {idx} frames to {output_path} (fps={fps})")

video_name = "cat_kiss.mp4"
output_name = video_name.rsplit(".", 1)[0] + ".bin"
video_to_container(f"tools/video/{video_name}", f"tools/output_video/{output_name}")
