from flask import Flask, request, render_template_string
import os
import pygame
import threading
from PIL import Image, ImageSequence
import requests
import io
import datetime
import time
from urllib.request import urlopen

# Settings
check_delay = 120  # minutes
rotate_delay = 5  # seconds
epic_images = []

# Initialize Flask and Pygame
app = Flask(__name__)
pygame.init()
os.environ["DISPLAY"] = ":0"
pygame.display.init()
screen = pygame.display.set_mode([480, 480], pygame.FULLSCREEN)
pygame.mouse.set_visible(0)
pygame.display.set_caption("GIF Viewer")

# Initialize lock and shared variables
gif_selection_lock = threading.Lock()
selected_gif = [None]  # Shared variable for gif selection


@app.route("/", methods=["GET", "POST"])
def index():
    if request.method == "POST":
        gif = request.form["gif"]
        with gif_selection_lock:
            selected_gif[0] = gif
    gifs = os.listdir("gifs/")
    gifs.append("epic")
    # Generate radio buttons for each GIF
    radio_buttons = "".join(
        f'<input type="radio" id="{gif}" name="gif" value="{gif}" {("checked" if selected_gif[0] == gif else "")}>'
        f'<label for="{gif}">{gif}</label><br>'
        for gif in gifs
    )
    form_html = f"""
    <form action="/" method="post">
        {radio_buttons}
        <button type="submit">Display GIF</button>
    </form>
    """
    return render_template_string(form_html)


def run_flask_app():
    app.run(host='0.0.0.0', port=5000, use_reloader=False, threaded=True)


def load_epic_frames():
    frames = [pygame.image.load(im) for im in epic_images]
    return frames


def poll_epic_images():
    global epic_images
    epic_images = ["images/loading.jpg"]
    last_check = datetime.datetime.now() - datetime.timedelta(days=1)
    last_data = ""
    newest_data = ""

    while True:
        if last_check >= (
            datetime.datetime.now() - datetime.timedelta(minutes=check_delay)
        ):
            time.sleep(2)
        else:
            last_check = datetime.datetime.now()
            print(str(last_check) + " Checking for new images.")

            # Call the epic api
            response = requests.get("https://epic.gsfc.nasa.gov/api/natural")
            imjson = response.json()
            newest_data = imjson[0]["date"]

            print("Last data: " + last_data)
            print("Newest data: " + newest_data)

            # If there are new images available, download them, then quickly display them all.
            if last_data == newest_data:
                print("No new images")
            else:
                print("Ooh! New Images!")
                last_data = newest_data

                images = []
                for i, photo in enumerate(imjson):
                    dt = datetime.datetime.strptime(photo["date"], "%Y-%m-%d %H:%M:%S")
                    imageurl = (
                        "https://epic.gsfc.nasa.gov/archive/natural/"
                        + str(dt.year)
                        + "/"
                        + str(dt.month).zfill(2)
                        + "/"
                        + str(dt.day).zfill(2)
                        + "/jpg/"
                        + photo["image"]
                        + ".jpg"
                    )

                    # Create a surface object, draw image on it..
                    image_file = io.BytesIO(urlopen(imageurl).read())
                    image = pygame.image.load(image_file)

                    # Crop out the centre 830px square from the image to make globe fill screen
                    cropped = pygame.Surface((830, 830))
                    cropped.blit(image, (0, 0), (125, 125, 830, 830))
                    cropped = pygame.transform.scale(cropped, (480, 480))

                    impath = f"images/{i}.jpg"
                    pygame.image.save(cropped, impath)

                    images.append(impath)

                epic_images = images
                print("Images saved")


def pygame_thread():
    running = True
    current_gif = ""
    gif_frames = []
    frame_index = 0
    frame_duration = -1
    last_frame_time = pygame.time.get_ticks()

    while running:
        current_time = pygame.time.get_ticks()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                pygame.quit()

        with gif_selection_lock:
            if selected_gif[0] and selected_gif[0] != current_gif:
                current_gif = selected_gif[0]
                if selected_gif[0] == "epic":
                    gif_frames = load_epic_frames()
                    frame_duration = rotate_delay * 1000
                else:
                    gif_frames = load_gif_frames(current_gif)
                    frame_duration = 100
                frame_index = 0
                last_frame_time = current_time

        if gif_frames:
            if frame_index == 0 or (current_time - last_frame_time > frame_duration):
                frame_index = (frame_index + 1) % len(gif_frames)
                last_frame_time = current_time
                screen.fill((0, 0, 0))
                screen.blit(gif_frames[frame_index], (0, 0))
                pygame.display.flip()


def load_gif_frames(gif_name):
    image_path = os.path.join("gifs", gif_name)
    pil_image = Image.open(image_path)
    frames = [
        pygame.image.fromstring(
            frame.copy().convert("RGBA").tobytes(), frame.size, "RGBA"
        )
        for frame in ImageSequence.Iterator(pil_image)
    ]
    resized_frames = [pygame.transform.scale(frame, (480, 480)) for frame in frames]
    return resized_frames


with gif_selection_lock:
    selected_gif[0] = "epic"

# Run Flask in a background thread
flask_thread = threading.Thread(target=run_flask_app, daemon=True)
flask_thread.start()

# Poll epi images in a background thread
epic_thread = threading.Thread(target=poll_epic_images, daemon=True)
epic_thread.start()

# Run Pygame in the main thread
pygame_thread()
