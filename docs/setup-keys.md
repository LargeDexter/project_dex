# Setting up Project Dex

Project Dex needs two things from Steam to build your library, plus one
optional extra for nicer box art. You only have to do this once -- enter
them on the Account Setup screen and they're saved for next time.

## 1. Steam Web API Key (required)

This lets Project Dex ask Steam for your owned-games list.

1. Go to **[steamcommunity.com/dev/apikey](https://steamcommunity.com/dev/apikey)** and sign in.
2. For "Domain Name," anything works -- `localhost` is fine, since this key
   isn't tied to a real website.
3. Click **Register**. You'll get a 32-character key like
   `1A2B3C4D5E6F7A8B9C0D1E2F3A4B5C6D`.
4. Type that into the **Steam Web API Key** field on the TV.

Keep this key private -- anyone with it can read (not change) your public
Steam data.

## 2. SteamID64 (required)

This tells Project Dex *which* Steam account's library to fetch -- your own.

1. Go to **[steamid.io](https://steamid.io)**.
2. Paste your Steam profile URL (or your profile's custom vanity name) into
   the search box.
3. Copy the **steamID64** value shown -- a 17-digit number starting with
   `7656119...`.
4. Type that into the **SteamID64** field on the TV.

Alternatively, if you're signed into Steam on this PC already, your
profile URL (Steam client -> your name in the top bar -> View my profile)
often shows this number directly if it hasn't been given a custom vanity
URL.

## 3. SteamGridDB API Key (optional)

Project Dex uses this for higher-quality box art. Without it, box art still
works -- it just falls back to Steam's own store artwork instead.

1. Go to **[steamgriddb.com](https://www.steamgriddb.com)** and create a
   free account (or sign in with Steam).
2. Go to your **Preferences -> API** page.
3. Generate a key and copy it.
4. Type that into the **SteamGridDB API Key** field on the TV, or leave it
   blank to skip this and use Steam's art instead.

---

Once all three fields look right (a green checkmark appears next to each),
select **Save** and Project Dex will sync your library right away.
