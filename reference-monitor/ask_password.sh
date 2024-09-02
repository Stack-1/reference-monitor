#!/bin/bash

get_password() {
  # Use `$TTY` if available (zsh), `$(tty)` otherwise (bash)
  TTY=${TTY:-"$(tty)"}
  {
    echo SETPROMPT "Password:"  # Set the prompt
    # Add more such customization commands here
    echo SETDESC "Enter password for reference monitor" # Set the description
    # Then finally ask for the password itself.
    echo GETPIN
  } |
    # pinentry assumes the frontend will be invoked in the background,
    # and needs to be explicitly told what environment it should use
    pinentry-curses --ttytype "$TERM" --ttyname "$TTY" --lc-ctype "$LC_CTYPE" --lc-messages "$LC_MESSAGES" |
    while IFS= read -r line; do
      case $line in
        OK*) continue;;
        D*) pass=${line#D }; printf "%s\n" "$pass";; # `D`ata line will have the password
        *) printf "%s\n" "$line" >&2;; # Dump everything else as possible errors 
      esac;
    done
}


password=$(get_password)

echo "$password"


