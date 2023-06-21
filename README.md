# M5Stack-AtomS3

## GIF source

https://emojipedia.org/animated-noto-color-emoji/

## Convert script

```
cd src
find * -exec convert {} -background black -coalesce -alpha remove ../out/{} \;
cd ../out
find * -exec ffmpeg -y -i {} -vf "fps=7,scale=128:128:flags=lanczos,split[s0][s1];[s0]palettegen=max_colors=32[p];[s1][p]paletteuse=dither=bayer" ../out2/{} \;
```