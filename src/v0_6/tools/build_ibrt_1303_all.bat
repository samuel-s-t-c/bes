@echo off


setlocal EnableDelayedExpansion

set TARGET_LIST=best2300a_ibrt_ai best2300a_ibrt_anc best2300a_ibrt
set PARAM_LIST=GOOGLE_SERVICE_ENABLE AMA_VOICE

if [%1] == [] (
    set DEBUG_CMD=
) else (
    set DEBUG_CMD=echo
)

%DEBUG_CMD% make T=best2300a_ibrt_ai allclean
for %%p in (%PARAM_LIST%) do (
    echo param is %%p
    echo.
    echo ------ Build best2300a_ibrt_ai ------
    echo.
    %DEBUG_CMD% make T=best2300a_ibrt_ai  %%p=1 OTA_ENABLE=1  INTERSYS_DEBUG=1 -j
    set err=!ERRORLEVEL!
    if !err! neq 0 (
        echo.
        echo ***********************
        echo Failed to build target: best2300a_ibrt_ai
        echo ***********************
        echo.
        exit /b !err!
    )
)

echo.
echo ------ Build all target list Multi ai ------
echo.
for %%t in (%TARGET_LIST%) do (
    echo.
    echo ------ Build %%t ------
    echo.
	%DEBUG_CMD% make T=%%t allclean
    %DEBUG_CMD% make T=%%t  AMA_VOICE=1 GOOGLE_SERVICE_ENABLE=1 TWS_PROMPT_SYNC=1  MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED=1 INTERSYS_DEBUG=1 -j
    set err=!ERRORLEVEL!
    if !err! neq 0 (
        echo.
        echo ***********************
        echo Failed to build target: %%t
        echo ***********************
        echo.
        exit /b !err!
    )
)

echo.
echo ------ Build all target list default ------
echo.
for %%t in (%TARGET_LIST%) do (
    echo.
    echo ------ Build %%t ------
    echo.
	%DEBUG_CMD% make T=%%t allclean
    %DEBUG_CMD% make T=%%t  TWS_PROMPT_SYNC=1  MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED=1 INTERSYS_DEBUG=1 -j
    set err=!ERRORLEVEL!
    if !err! neq 0 (
        echo.
        echo ***********************
        echo Failed to build target: %%t
        echo ***********************
        echo.
        exit /b !err!
    )
)
echo.
echo -----------------------------------------
echo Successfully build the following targets:
for %%t in (%TARGET_LIST%) do (
    echo     %%t
)

