for f in *.wav; do
  ffmpeg -i "$f" -b:a 128k "${f%.wav}.mp3"
done
