from flask import Flask, request, render_template_string
import os
import pygame
import threading
from PIL import Image, ImageSequence
from urllib.request import urlopen
import requests
import io
import datetime
import time
from functools import lru_cache
from bidict import bidict
import pytz

# Settings
check_delay = 15*60 # seconds
rotate_delay = 4  # seconds
first_time = [True]
CACHE = bidict()
CACHE_LOCK = threading.Lock()

# Pi Zero-friendly caches: keep them small to avoid memory pressure.
GIF_FRAME_CACHE_SIZE = 4
IMAGE_SURFACE_CACHE_SIZE = 8
_IMAGE_PATHS_CACHE = {}  # mode -> (signature, [paths])

HTTP_TIMEOUT_S = 20
HTTP = requests.Session()

# Initialize Flask and Pygame
app = Flask(__name__)
pygame.init()
os.environ["DISPLAY"] = ":0"
pygame.display.init()

screen = pygame.display.set_mode([480, 480], pygame.FULLSCREEN)
# screen = pygame.display.set_mode([480, 480])
pygame.mouse.set_visible(0)
pygame.display.set_caption("GIF Viewer")

# Initialize lock and shared variables
gif_selection_lock = threading.Lock()
selected_gif = ["rammb"]
gif_changed = [True]

RAMMB_BASE_URL = "https://rammb-slider.cira.colostate.edu/data/"

@app.route("/", methods=["GET", "POST"])
def index():
    if request.method == "POST":
        gif = request.form["gif"]
        with gif_selection_lock:
            if gif != selected_gif[0]:
                selected_gif[0] = gif
                gif_changed[0] = True

    gifs = os.listdir("gifs/")
    gifs.extend(["epic", "rammb"])
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
    app.run(host="0.0.0.0", port=5000, use_reloader=False, threaded=True)


def load_image_frames():
    if first_time[0]:
        return [pygame.image.load("images/loading.jpg")]

    mode = selected_gif[0]
    with CACHE_LOCK:
        image_filenames = [
            impath
            for (impath, src) in sorted(CACHE.items(), key=lambda x: x[1])
            if mode in impath
        ]

    # Signature based on filenames + mtimes so we can reuse the resolved list
    signature_parts = []
    existing_files = []
    for impath in image_filenames:
        try:
            signature_parts.append((impath, os.path.getmtime(impath)))
            existing_files.append(impath)
        except FileNotFoundError:
            # If a file was pruned/overwritten between building the list and now,
            # just skip it and let the next refresh rebuild cleanly.
            continue
    signature = tuple(signature_parts)

    cached = _IMAGE_PATHS_CACHE.get(mode)
    if cached and cached[0] == signature:
        return cached[1]

    # On constrained devices: return paths, and load surfaces lazily with a tiny LRU.
    _IMAGE_PATHS_CACHE[mode] = (signature, existing_files)
    return existing_files


@lru_cache(maxsize=IMAGE_SURFACE_CACHE_SIZE)
def _load_surface_cached(image_path: str, mtime: float):
    surf = pygame.image.load(image_path)
    # `convert()` makes blitting much faster because it matches the display format.
    if image_path.lower().endswith(".png"):
        return surf.convert_alpha()
    return surf.convert()


def _prune_cached_images(prefix: str, expected_paths: set[str]) -> int:
    """
    Remove stale CACHE entries (and their files) for a given filename prefix.
    This prevents showing old frames when the upstream returns fewer images.
    """
    stale_paths: list[str] = []
    with CACHE_LOCK:
        for impath in list(CACHE.keys()):
            if impath.startswith(prefix) and impath not in expected_paths:
                stale_paths.append(impath)
                try:
                    del CACHE[impath]
                except KeyError:
                    pass

    for impath in stale_paths:
        try:
            os.remove(impath)
        except FileNotFoundError:
            pass

    # Invalidate in-memory loaded frames for the corresponding mode
    # (mode string is embedded in the prefix, e.g. "images/epic_")
    if "epic_" in prefix:
        _IMAGE_PATHS_CACHE.pop("epic", None)
    if "rammb_" in prefix:
        _IMAGE_PATHS_CACHE.pop("rammb", None)
    return len(stale_paths)


def get_latest_epic_urls():
    response = HTTP.get("https://epic.gsfc.nasa.gov/api/natural", timeout=HTTP_TIMEOUT_S)
    imjson = response.json()
    newest_data = imjson[0]["date"]

    urls = []
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

        urls.append((dt, imageurl))
    return newest_data, urls


def get_latest_rammb_urls(sat="meteosat-0deg", sector="full_disk", product="geocolor", limit=10,
                          zoom=0, tile_x_filter=[], tile_y_filter=[]):

    TILE_SIZE = 464
    ZOOM_TILES = [1, 2, 4, 8, 16]
    timestamps_url = f"{RAMMB_BASE_URL}json/{sat}/{sector}/{product}/latest_times.json"
    response = HTTP.get(timestamps_url, timeout=HTTP_TIMEOUT_S)
    tsjson = response.json()

    latest_times = sorted(tsjson["timestamps_int"])

    if limit:
        latest_times = latest_times[-limit:]

    newest_data = str(latest_times[-1])

    print(f"Checked {timestamps_url}, all data is {latest_times}")
    print(f"latest data available is {newest_data}")

    x_tiles = range(0, ZOOM_TILES[zoom])
    y_tiles = range(0, ZOOM_TILES[zoom])
    if tile_x_filter: x_tiles = [x for x in x_tiles if x in tile_x_filter]
    if tile_y_filter: y_tiles = [x for x in y_tiles if x in tile_y_filter]
    n_x_tiles = len(x_tiles)
    n_y_tiles = len(y_tiles)

    urls = []
    for ts in latest_times:
        dt = datetime.datetime.strptime(str(ts), "%Y%m%d%H%M%S")
        tiles = []
        for tile_x in x_tiles:
            for tile_y in y_tiles:
                imageurl = f'{RAMMB_BASE_URL}imagery/{dt.strftime("%Y/%m/%d")}/{sat}---{sector}/{product}/{ts}/{zoom:02}/{tile_y:03}_{tile_x:03}.png'
                tiles.append(imageurl)
    
        urls.append((dt, tiles, (len(tiles), n_x_tiles, n_y_tiles, TILE_SIZE)))

    return newest_data, urls

def poll_images_thread():

    while True:
        print(datetime.datetime.now())
        print("Polling for images...")
        # poll_epic_images()
        poll_rammb_images()

        if first_time[0]:
            first_time[0] = False
            gif_changed[0] = True
            print("First set of images downloaded")

        time.sleep(check_delay)

def stitch_tiles(tile_urls, tile_size, tiles_x, tiles_y):
    # Create a blank canvas for the final image
    final_image = pygame.Surface((tiles_x * tile_size, tiles_y * tile_size))

    # Loop through the tiles and paste them in the correct position
    for i, url in enumerate(tile_urls):
        # Fetch the image from the URL
        print(f"Downloading tile {url.replace(RAMMB_BASE_URL,'')}")
        response = HTTP.get(url, timeout=HTTP_TIMEOUT_S)
        tile_image = pygame.image.load(io.BytesIO(response.content)).convert()

        # Calculate the row and column positions (assuming row-major order)
        col = i // tiles_y 
        row = i % tiles_y 

        # Calculate the pixel position in the final image
        x_pos = col * tile_size
        y_pos = row * tile_size

        # Paste the tile image into the final image
        final_image.blit(tile_image, (x_pos, y_pos))

    return final_image

def overlay_date(dt, image):
    utc_dt = dt.replace(tzinfo=pytz.UTC)
    uk_tz = pytz.timezone('Europe/London')
    uk_dt = utc_dt.astimezone(uk_tz)
    current_date = uk_dt.strftime("%a %d %b %Y %H:%M")

    # Set up the font and size
    font = pygame.font.Font(None, 24)
    text = font.render(current_date, True, (255, 255, 255))  # White color

    # Get the text's rectangular area and position it at the bottom center
    text_rect = text.get_rect()
    text_rect.centerx = image.get_rect().centerx
    text_rect.y = image.get_rect().bottom - 30

    # Blit the text onto the image
    image.blit(text, text_rect)

    return image

def poll_epic_images():
    print(f"Checking for new images.")

    newest_date, urls = get_latest_epic_urls()

    new_data = False
    ims = []
    for i, (dt, imageurl) in enumerate(urls):

        with CACHE_LOCK:
            seen = imageurl in CACHE.inverse
        if seen:
            # We've handled this url already
            print(f" Cache hit {imageurl.replace(RAMMB_BASE_URL,'')}")
            continue

        new_data = True

        # Simple case, no tiling
        print(f" Downloading simple image {imageurl.replace(RAMMB_BASE_URL,'')}")
        image_file = io.BytesIO(urlopen(imageurl).read())
        image = pygame.image.load(image_file)
    
        # Crop out the centre 830px square from the image to make globe fill screen
        cropped = pygame.Surface((830, 830))
        cropped.blit(image, (0, 0), (125, 125, 830, 830))
        cropped = pygame.transform.scale(cropped, (480, 480))

        cropped = overlay_date(dt, cropped)
        impath = f"images/epic_{i}.jpg"
        pygame.image.save(cropped, impath)
        with CACHE_LOCK:
            CACHE[impath] = imageurl

    expected = {f"images/epic_{i}.jpg" for i in range(len(urls))}
    pruned = _prune_cached_images("images/epic_", expected)
    if pruned:
        new_data = True

    print(f"{len(urls)} images for epic saved")

    if new_data:
        with gif_selection_lock:
            print(f"Some new images detected")
            gif_changed[0] = True


def poll_rammb_images():
    print(f"Checking for new images.")
    new_data = False
    i = -1
    newest_date, world_urls = get_latest_rammb_urls()
    
    for dt, tiles, (nt, nx, ny, tile_size) in world_urls:
        i += 1
        imageurl = tiles[0]
        with CACHE_LOCK:
            seen = imageurl in CACHE.inverse
        if seen:
            # We've handled this url or set of tiles already
            print(f" Cache hit {imageurl.replace(RAMMB_BASE_URL,'')}")
            continue
        
        new_data = True

        # Simple case, no tiling
        print(f" Downloading simple image {imageurl.replace(RAMMB_BASE_URL,'')}")
        image_file = io.BytesIO(urlopen(imageurl).read())
        image = pygame.image.load(image_file)
        cropped = pygame.transform.scale(image, (480, 480))

        cropped = overlay_date(dt, cropped)
        impath = f"images/rammb_{i}.jpg"
        pygame.image.save(cropped, impath)
        with CACHE_LOCK:
            CACHE[impath] = imageurl

    print(f"{len(world_urls)} rammb world images saved")

    newest_date, europe_urls = get_latest_rammb_urls(zoom=3,
                                           tile_x_filter=range(3,6),
                                           tile_y_filter=range(0,2))
    
    for dt, tiles, (nt, nx, ny, tile_size) in europe_urls:
        i += 1  
        imageurl = tiles[0]
        # Add a prefix to separate them from the world urls
        with CACHE_LOCK:
            seen = ("eu_" + imageurl) in CACHE.inverse
        if seen:
            # We've handled this url or set of tiles already
            print(f" Cache hit {imageurl.replace(RAMMB_BASE_URL,'')}")
            continue
        
        new_data = True

        # Set of image tiles
        print(f" Downloading tiled image")
        image = stitch_tiles(tiles, tile_size, nx, ny)
        # Crop out Europe (X, Y, Width, Height)
        crop_box = (102, 21, 102 + 879, 21 + 879)
        crop_rect = pygame.Rect(*crop_box)
        # Crop the image using subsurface
        cropped = image.subsurface(crop_rect)
        cropped = pygame.transform.scale(cropped, (480, 480))

        cropped = overlay_date(dt, cropped)
        impath = f"images/rammb_{i}.jpg"
        pygame.image.save(cropped, impath)
        with CACHE_LOCK:
            CACHE[impath] = ("eu_" + imageurl)

    print(f"{len(europe_urls)} images for ramb europe saved")

    # Prune any stale `images/rammb_*.jpg` that were cached previously but not
    # produced in this poll cycle (e.g. if upstream returns fewer frames).
    expected = {f"images/rammb_{j}.jpg" for j in range(i + 1)}
    pruned = _prune_cached_images("images/rammb_", expected)
    if pruned:
        new_data = True

    if new_data:
        with gif_selection_lock:
            print(f"Some new images detected")
            gif_changed[0] = True


def pygame_thread():
    running = True
    current_gif = ""
    gif_frames = []  # can be Surfaces (GIFs/loading) or file paths (epic/rammb)
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
            if selected_gif[0] and gif_changed[0]:
                current_gif = selected_gif[0]
                if selected_gif[0].startswith("epic") or selected_gif[0].startswith(
                    "rammb"
                ):
                    gif_frames = load_image_frames()
                    frame_duration = rotate_delay * 1000
                else:
                    gif_frames = load_gif_frames(current_gif)
                    frame_duration = 100
                frame_index = 0
                last_frame_time = current_time
                gif_changed[0] = False

        if gif_frames:
            if frame_index == 0 or (current_time - last_frame_time > frame_duration):
                frame_index = (frame_index + 1) % len(gif_frames)
                last_frame_time = current_time
                screen.fill((0, 0, 0))
                frame = gif_frames[frame_index]
                if isinstance(frame, str):
                    try:
                        mtime = os.path.getmtime(frame)
                        frame_surf = _load_surface_cached(frame, mtime)
                    except FileNotFoundError:
                        # If a frame disappears (prune/refresh), force reload on next tick.
                        with gif_selection_lock:
                            gif_changed[0] = True
                        continue
                    screen.blit(frame_surf, (0, 0))
                else:
                    screen.blit(frame, (0, 0))
                pygame.display.flip()


def load_gif_frames(gif_name):
    image_path = os.path.join("gifs", gif_name)
    mtime = os.path.getmtime(image_path)
    return list(_load_gif_frames_cached(image_path, mtime))


@lru_cache(maxsize=GIF_FRAME_CACHE_SIZE)
def _load_gif_frames_cached(image_path: str, mtime: float):
    # `mtime` is part of the cache key so edits to a GIF invalidate the cache.
    pil_image = Image.open(image_path)
    frames = []
    for frame in ImageSequence.Iterator(pil_image):
        surf = pygame.image.fromstring(
            frame.copy().convert("RGBA").tobytes(), frame.size, "RGBA"
        )
        frames.append(pygame.transform.scale(surf, (480, 480)))
    return tuple(frames)


# Poll images in a background thread
epic_thread = threading.Thread(target=poll_images_thread, daemon=True)
epic_thread.start()

# Run Flask in a background thread
flask_thread = threading.Thread(target=run_flask_app, daemon=True)
flask_thread.start()

# Run Pygame in the main thread
pygame_thread()
