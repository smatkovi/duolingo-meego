# Aufgabenarten und ihre Benotung

Erzeugt von `tools/aufgaben-lesen.py` aus dem Web-Buendel -- nicht von Hand gepflegt.

`grade` ist die Regel, nach der die **Weboberflaeche selbst** benotet; der Server uebernimmt das Ergebnis beim `PUT /sessions/{id}`. Unsere Fassung muss dasselbe rechnen.


Gefunden: **64** Aufgabenarten.


## `assist` (Assist)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:t.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.options.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.text,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `characterIntro` (CharacterIntro)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:`${t.guess}`})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>[{autoplay:!0,required:!1,text:e.prompt,url:e.tts}]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `characterMatch` (CharacterMatch)

- `grade`: `({challengeState:e})=>Promise.resolve({correct:void 0!==e.guess,guess:e.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.pairs.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.character,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `characterPuzzle` (CharacterPuzzle)

- `grade`: `({challenge:e,challengeState:t})=>{var a;const s=null===(a=t.guess)||void 0===a?void 0:a.choices.map((t=>t>=0?e.choices[t].text:"")).join(""),n=e.correctIndices.map((t=>e.choices[t].text)).join("");return Promise.resolve({correct:s===n,guess:s})}`
- `validate`: `({challenge:e,challengeState:t})=>{var a,s;return(null===(s=null===(a=t.guess)||void 0===a?void 0:a.choices)||void 0===s?void 0:s.filter((e=>e>=0)).length)===e.correctIndices.length}`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA([...e.choices.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.text,url:e.tts})),{autoplay:!0,required:!1,text:e.correctSolutions[0],url:e.tts}])`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `characterSelect` (CharacterSelect)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:`${t.guess}`})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.choices.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.character,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `characterTrace` (CharacterTrace)

- `grade`: `({challenge:e,challengeState:t})=>{var a,s;return Promise.resolve({correct:(null!==(s=null===(a=t.guess)||void 0===a?void 0:a.currentStrokeIndex)&&void 0!==s?s:0)>=e.strokes.length,guess:t.guess})}`
- `validate`: `({challenge:e,challengeState:t})=>{var a,s;return(null!==(s=null===(a=t.guess)||void 0===a?void 0:a.currentStrokeIndex)&&void 0!==s?s:0)>=e.strokes.length}`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>e.tts?[{autoplay:!0,required:!1,text:e.prompt,url:e.tts}]:[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `characterWrite` (CharacterWrite)

- `grade`: `({challenge:e,challengeState:t})=>{var a,s,n;const r=e.strokes.filter((e=>"PREDRAWN"!==e.strokeDrawMode)),i=null!==(s=null===(a=t.guess)||void 0===a?void 0:a.currentStrokeIndex)&&void 0!==s?s:0,o=i>=r.length&&!(null===(n=t.guess)||void 0===n?void 0:n.skippedCurrentStroke);return Promise.resolve({correct:o,guess:t.guess,numStrokesLeft:r.length-i})}`
- `validate`: `({challenge:e,challengeState:t})=>{var a,s;const n=e.strokes.filter((e=>"PREDRAWN"!==e.strokeDrawMode));return(null!==(s=null===(a=t.guess)||void 0===a?void 0:a.currentStrokeIndex)&&void 0!==s?s:0)>=n.length}`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>e.promptTts?[{autoplay:!0,required:!1,text:e.prompt,url:e.promptTts}]:[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `completeReverseTranslation` (CompleteReverseTranslation)

- `grade`: `async({challenge:e,challengeState:t,challengeToggleState:a})=>{if(a.isToggledToTyping){const s=this.getTypedSentence(t);return(0,g.wM)({allowTypoBlame:!0,challenge:e,guess:s,isTypingInLearningLanguage:a.isToggledToTyping})}const n=this.getTypedWords(e,t),r=this.getAllWords(e,t).join(""),i=e.displayTokens.filter((e=>e.isBlank)).map((e=>e.text));if(s.Xy(n,i))return Promise.resolve({correct:!0,guess:r});const o=await(0,g.wM)({allowTypoBlame:!0,challenge:e,guess:r,isTypingInLearningLanguage:a.isToggledToTyping});return o.correct||delete o.blameMessage,o}`
- `validate`: `({challenge:e,challengeState:t,challengeToggleState:a})=>a.isToggledToTyping?this.getTypedSentence(t).length>0:this.getTypedWords(e,t).every((e=>e))`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tokens.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({guess:{translatedWords:e.displayTokens.map((()=>""))},type:e.type})`

## `definition` (Definition)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:t.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA([...e.displayTokens.map((e=>e.hintToken.tts&&{autoplay:!1,required:!1,text:e.text,url:e.hintToken.tts})),e.tts&&{autoplay:!0,required:!1,text:e.displayTokens.map((e=>e.text)).join(""),url:e.tts}])`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `dialogue` (Dialogue)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:`${t.guess}`})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `({challenge:e,challengeState:t})=>{const{dialogue:a}=e,{currentDialogueIndex:s}=t;return s>=a.length}`
- `getTts`: `({challenge:e})=>{var t;return s.oA([...e.dialogue.map((e=>e.hintTokens)).flat(),...null!==(t=e.promptTokens)&&void 0!==t?t:[]].map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))}`
- `prepareChallengeState`: `({challenge:e,isDesktop:t})=>({currentDialogueIndex:t?e.dialogue.length:0,type:e.type})`

## `extendedMatch` (ExtendedMatch)

- `grade`: `({challengeState:e})=>Promise.resolve({correct:void 0!==e.guess,guess:e.guess})`
- `validate`: `()=>!1`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.pairs.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.learningToken,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `extendedListenMatch` (ExtendedListenMatch)

- `grade`: `({challengeState:e})=>Promise.resolve({correct:void 0!==e.guess,guess:e.guess})`
- `validate`: `()=>!1`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.pairs.map((e=>e.tts&&{autoplay:!1,required:!0,text:e.translation,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `form` (Form)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,gradingMetadata:(0,E.u5)(e,t.guess),guess:`${t.guess}`})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `freeResponse` (FreeResponse)

- `grade`: `({challengeState:e})=>Promise.resolve({correct:""!==this.getResponse(e),guess:this.getResponse(e)})`
- `validate`: `({challenge:e,challengeState:t})=>{const{maxGuessLength:a}=e,s=this.getResponse(t);return s.length>0&&s.length<=a}`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `gapFill` (GapFill)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({closestSolution:e.displayTokens.map((e=>e.text)).join(""),correct:t.guess===e.correctIndex,guess:void 0===t.guess?void 0:e.choices[t.guess]})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tokens.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `judge` (Judge)

- `grade`: `({challenge:e,challengeState:t})=>(e.correctIndices.length>1&&(0,h.vU)(`Judge has more than one right answer: ${e.prompt}`),Promise.resolve({correct:void 0!==t.guess&&e.correctIndices.includes(t.guess),guess:t.guess}))`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `listen` (Listen)

- `grade`: `({challenge:e,challengeState:t,learningLanguage:a})=>(0,g.wM)({allowTypoBlame:!0,challenge:e,guess:this.getGuess(t),isTypingInLearningLanguage:e.grader.language===a})`
- `validate`: `({challengeState:e})=>!!this.getGuess(e)`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA([e.slowTts&&{autoplay:!1,required:!1,text:e.prompt,url:e.slowTts},{autoplay:!0,required:!0,text:e.prompt,url:e.tts}])`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `listenComplete` (ListenComplete)

- `grade`: `({challenge:e,challengeState:t,learningLanguage:a})=>(0,g.wM)({allowTypoBlame:!0,challenge:e,guess:this.getUserInput(e,t,!0).join(""),isTypingInLearningLanguage:e.grader.language===a})`
- `validate`: `({challenge:e,challengeState:t})=>this.getUserInput(e,t,!1).every((e=>e))`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA([e.slowTts&&{autoplay:!1,required:!1,text:e.prompt,url:e.slowTts},{autoplay:!0,required:!0,text:e.prompt,url:e.tts}])`
- `prepareChallengeState`: `({challenge:e})=>({guess:e.displayTokens.map((()=>"")),type:e.type})`

## `listenMatch` (ListenMatch)

- `grade`: `({challengeState:e})=>Promise.resolve({correct:void 0!==e.guess,guess:e.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.pairs.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.translation,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `match` (Match)

- `grade`: `({challengeState:e})=>Promise.resolve({correct:void 0!==e.guess,guess:e.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.pairs.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.learningToken,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `name` (Name)

- `grade`: `({challenge:e,challengeState:t,session:a})=>{var s;const n=[this.getTranslation(t)],i=this.getArticle(e,t);if(i&&n.unshift(i),e.grader){const t=null!==(s=(0,c.KV)(n))&&void 0!==s?s:"",a={...e,grader:e.grader};return(0,g.wM)({allowTypoBlame:!0,challenge:a,guess:t,isTypingInLearningLanguage:!0})}const o=(e.correctSolutions||[]).map((e=>(0,d.lG)(j,e)));if(o.includes((0,d.lG)(j,n.join(B)))||o.includes((0,d.lG)(j,n.join(""))))return Promise.resolve({correct:!0,guess:n});const l=(0,I.globalGetState)();let u=!1;if(l){const e=(0,r.HF)(l),t=a.skillId;t&&e&&(u=void 0!==(0,y.m8)(e,(e=>"skill"===e.type&&e.pathLevelClientData.skillId===t&&"custom_intro"===e.subtype)))}if(u)return Promise.resolve({correct …`
- `validate`: `({challenge:e,challengeState:t})=>(!e.articles||this.getArticle(e,t).length>0)&&this.getTranslation(t).length>0`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `listenComprehension` (ListenComprehension)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:`${t.guess}`})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>{var t;return s.oA([e.slowTts&&{autoplay:!1,required:!1,text:e.prompt,url:e.slowTts},{autoplay:!0,required:!0,text:e.prompt,url:e.tts},...(null!==(t=e.questionTokens)&&void 0!==t?t:[]).map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts}))])}`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `listenIsolation` (ListenIsolation)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:`${t.guess}`})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>{var t,a;return s.oA([{autoplay:!0,required:!0,text:e.tokens.map((e=>e.value)).join(""),url:e.tts},...(null!==(t=e.options)&&void 0!==t?t:[]).map((e=>e.tts&&{autoplay:!1,required:!0,text:e.text,url:e.tts})),...(null!==(a=e.tokens)&&void 0!==a?a:[]).map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts}))])}`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `listenSpeak` (ListenSpeak)

- `grade`: `({challenge:e,challengeState:t})=>{var a,s,n,r;const i=(null!==(s=null===(a=t.guess)||void 0===a?void 0:a.score)&&void 0!==s?s:0)>=e.threshold;return Promise.resolve({correct:i,guess:t.guess,numRetriesLeft:i||void 0===(null===(n=t.guess)||void 0===n?void 0:n.usedGuesses)?0:l.Tu-(null===(r=t.guess)||void 0===r?void 0:r.usedGuesses)})}`
- `validate`: `()=>!1`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>[{autoplay:!0,required:!1,text:e.prompt,url:e.tts}]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `listenTap` (ListenTap)

- `grade`: `async({challenge:e,challengeState:t,challengeToggleState:a})=>{if(a.isToggledToTyping)return(0,g.wM)({allowTypoBlame:!0,challenge:e,guess:this.getTranscription(t),isTypingInLearningLanguage:a.isToggledToTyping});const n=(0,l.rF)(e),r=this.getTapTranscriptions(e,t),i=r.join(n),o=e.correctIndices.map((t=>e.choices[t].text));if(s.Xy(o,r))return Promise.resolve({correct:!0,guess:i});const c=await(0,g.wM)({allowTypoBlame:!1,challenge:e,guess:i,isTypingInLearningLanguage:a.isToggledToTyping});return c.correct?c:Promise.resolve({correct:c.correct,guess:i})}`
- `validate`: `({challenge:e,challengeState:t,challengeToggleState:a})=>a.isToggledToTyping?this.getTranscription(t).length>0:this.getTapTranscriptions(e,t).length>0`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA([e.slowTts&&{autoplay:!1,required:!1,text:e.prompt,url:e.slowTts},{autoplay:!0,required:!0,text:e.prompt,url:e.tts},...e.choices.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.text,url:e.tts}))])`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `mathChallengeBlob` (MathChallengeBlob)

- `grade`: `async({challenge:e,challengeState:t,session:a})=>{var s;const n=e.challengeBlob;if("tutor"===n.layout)return{blameSubtextFormat:"latex",correct:!0,guess:t.guess};if("match"===n.layout||"matchMadness"===n.layout){const s=(()=>{if(e.gradingFeedbackBlobOverride)try{const t=JSON.parse(e.gradingFeedbackBlobOverride);if(C.Jm(t)&&t.grading_feedback)return V(t.grading_feedback,{challengeId:e.id,isLlmGenerated:a.isLlmGenerated,skillId:a.skillId})}catch(e){(0,h.Kp)("Failed to parse match grading feedback blob",{ex:e,once:!0})}})(),n=await(null==s?void 0:s(t.guess));return{blameSubtextFormat:"latex",correct:void 0!==t.guess,guess:t.guess,longFormFeedbackData:n}}if("multiStep"===n.layout)return(0,S.yX)( …`
- `validate`: `({challenge:e,challengeState:t})=>(0,f.G)(e.challengeBlob,t.guess)`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `orderTapComplete` (OrderTapComplete)

- `grade`: `({challenge:e,challengeState:t})=>{const a=this.getGuesses(t).values();let s=!0,n=!1;for(const t of e.displayTokens)if(t.isBlank){n=!0;const r=a.next().value;if(null==r||t.text!==e.choices[r].text){s=!1;break}}else if(n&&t.text!==B){n=!1;const e=a.next();if(null!==e.value&&!e.done){s=!1;break}}return Promise.resolve({closestSolution:e.displayTokens.map((e=>e.text)).join(""),correct:s,guess:t.guess,highlights:this.getHighlights(e,t)})}`
- `validate`: `({challengeState:e})=>this.getGuesses(e).length>0&&null!==this.getGuesses(e)[0]`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tokens.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `partialListen` (PartialListen)

- `grade`: `async({challenge:e,challengeState:t,challengeToggleState:a})=>{var n;const r=e.displayTokens,i=r.findIndex((e=>e.isBlank)),o=e.displayTokens.map((e=>e.text)).join(""),l=null!==(n=t.guess)&&void 0!==n?n:"",c=r.map(((e,t)=>e.isBlank?t===i?l:"":e.text)).join("");if(s.Xy(c,o))return Promise.resolve({correct:!0,guess:l});const d=await(0,g.wM)({allowTypoBlame:!0,challenge:e,guess:c,isTypingInLearningLanguage:a.isToggledToTyping});return d.guess=l,d.correct||delete d.blameMessage,d}`
- `validate`: `({challengeState:e})=>!!e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `partialReverseTranslate` (PartialReverseTranslate)

- `grade`: `async({challenge:e,challengeState:t,challengeToggleState:a})=>{var n;const r=e.displayTokens,i=r.findIndex((e=>e.isBlank)),o=e.displayTokens.map((e=>e.text)).join(""),l=null!==(n=t.guess)&&void 0!==n?n:"",c=r.map(((e,t)=>e.isBlank?t===i?l:"":e.text)).join("");if(s.Xy(c,o))return Promise.resolve({correct:!0,guess:l});const d=await(0,g.wM)({allowTypoBlame:!0,challenge:e,guess:c,isTypingInLearningLanguage:a.isToggledToTyping});return d.guess=l,d.correct||delete d.blameMessage,d}`
- `validate`: `({challengeState:e})=>!!e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tokens.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `patternTapComplete` (PatternTapComplete)

- `grade`: `async({challenge:e,challengeState:t})=>Promise.resolve({correct:e.correctIndex===t.guess,guess:void 0===t.guess?"":e.choices[t.guess]})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `radioBinary` (RadioBinary)

- `grade`: `({challengeState:e})=>Promise.resolve({correct:!0,guess:e.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `radioImageSelect` (RadioImageSelect)

- `grade`: `({challengeState:e})=>Promise.resolve({correct:!0,guess:e.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `radioListenMatch` (RadioListenMatch)

- `grade`: `({challengeState:e})=>Promise.resolve({correct:!0,guess:e.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({guess:void 0,pairs:e.pairs,type:e.type})`

## `radioListenRecognize` (RadioListenRecognize)

- `grade`: `({challengeState:e})=>Promise.resolve({correct:!0,guess:e.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({correctIndices:!0,guess:e.correctIndices,type:e.type})`

## `radioSelect` (RadioSelect)

- `grade`: `({challengeState:e})=>Promise.resolve({correct:!0,guess:e.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `readComprehension` (ReadComprehension)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:`${t.guess}`})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `({challengeState:e})=>e.finishedPassage`
- `getTts`: `({challenge:e})=>{var t,a;return s.oA([...null!==(t=e.passageTokens)&&void 0!==t?t:[],...null!==(a=e.questionTokens)&&void 0!==a?a:[]].map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))}`
- `prepareChallengeState`: `({challenge:e,isDesktop:t})=>({finishedPassage:t,type:e.type})`

## `reverseAssist` (ReverseAssist)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:t.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `sameDifferent` (SameDifferent)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:t.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.ttsURLs.map(((t,a)=>({autoplay:!0,required:!0,text:e.prompts[a],url:t}))))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `select` (Select)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:`${t.guess}`})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.choices.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.phrase,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `selectPronunciation` (SelectPronunciation)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:`${t.guess}`})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>e.choices.map(((t,a)=>({autoplay:a===e.correctIndex,required:!0,text:t.text,url:t.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `selectTranscription` (SelectTranscription)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:`${t.guess}`})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA([{autoplay:!0,required:!0,text:e.choices[e.correctIndex].text,url:e.tts},...e.choices.map((e=>e.tts&&{autoplay:!1,required:!0,text:e.text,url:e.tts}))])`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `svgPuzzle` (SvgPuzzle)

- `grade`: `({challenge:e,challengeState:t})=>{var a,s;let n=!0;for(let s=0;s<e.correctIndices.length;s++)if(e.correctIndices[s]!==(null===(a=t.guess)||void 0===a?void 0:a.choices[s])){n=!1;break}const r=null===(s=t.guess)||void 0===s?void 0:s.choices.map((e=>`${e}`)).join(", ").trim();return Promise.resolve({correct:n,guess:`[${r}]`})}`
- `validate`: `({challenge:e,challengeState:t})=>{var a,s;return(null===(s=null===(a=t.guess)||void 0===a?void 0:a.choices)||void 0===s?void 0:s.filter((e=>e>=0)).length)===e.correctIndices.length}`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.choices.map((e=>e.tts&&e.text&&{autoplay:!1,required:!1,text:e.text,url:e.tts})).concat(e.tts?[{autoplay:!0,required:!1,text:e.correctSolutions[0],url:e.tts}]:[]))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `syllableTap` (SyllableTap)

- `grade`: `e=>W(e)`
- `validate`: `e=>$(e)`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA([{autoplay:!1,required:!0,text:e.prompt,url:e.tts},...e.choices.map((e=>e.tts&&{autoplay:!1,required:!0,text:e.text,url:e.tts}))])`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `syllableListenTap` (SyllableListenTap)

- `grade`: `e=>W(e)`
- `validate`: `e=>$(e)`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA([{autoplay:!1,required:!0,text:e.prompt,url:e.tts},...e.choices.map((e=>e.tts&&{autoplay:!1,required:!0,text:e.text,url:e.tts}))])`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `speak` (Speak)

- `grade`: `({challenge:e,challengeState:t})=>{var a,s,n,r;const i=(null!==(s=null===(a=t.guess)||void 0===a?void 0:a.score)&&void 0!==s?s:0)>=e.threshold;return Promise.resolve({correct:i,guess:t.guess,numRetriesLeft:i||void 0===(null===(n=t.guess)||void 0===n?void 0:n.usedGuesses)?0:l.Tu-(null===(r=t.guess)||void 0===r?void 0:r.usedGuesses)})}`
- `validate`: `()=>!1`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>[{autoplay:!0,required:!1,text:e.prompt,url:e.tts}]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `tapCloze` (TapCloze)

- `grade`: `({challenge:e,challengeState:t})=>{const a=this.getGuessedTexts(e,t),n=(0,p.Ub)(e);return Promise.resolve({correct:s.Xy(a,n),guess:a.join(""),highlights:this.getHighlights(e,t)})}`
- `validate`: `({challenge:e,challengeState:t})=>this.getGuesses(e,t).every((e=>void 0!==e))`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tokens.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({guess:(0,p.jL)(e.displayTokens),type:e.type})`

## `tapClozeTable` (TapClozeTable)

- `grade`: `({challenge:e,challengeState:t})=>{const a=this.getGuessedTexts(e,t),n=(0,p.Ub)(e);return Promise.resolve({correct:s.Xy(a,n),guess:a.join(""),highlights:this.getHighlights(e,t)})}`
- `validate`: `({challenge:e,challengeState:t})=>this.getGuesses(e,t).every((e=>void 0!==e.guess))`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tableTokens.flat(2).map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({guess:(0,p.Xh)(e.displayTableTokens),type:e.type})`

## `tapComplete` (TapComplete)

- `grade`: `({challenge:e,challengeState:t})=>{const a=this.getGuesses(e,t),n=a.values(),r=e.displayTokens.map((t=>{if(t.isBlank){const t=n.next().value;return null==t?"":e.choices[t].text}return t.text})).join("");return Promise.resolve({closestSolution:e.displayTokens.map((e=>e.text)).join(""),correct:s.Xy(a,e.correctIndices),guess:r,highlights:this.getHighlights(e,t)})}`
- `validate`: `({challenge:e,challengeState:t})=>this.getGuesses(e,t).filter((e=>null!==e)).length===e.correctIndices.length`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tokens.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `tapCompleteTable` (TapCompleteTable)

- `grade`: `({challenge:e,challengeState:t})=>{const a=this.getGuessedTexts(e,t),n=(0,p.Ub)(e);return Promise.resolve({correct:s.Xy(a,n),guess:a.join(""),highlights:this.getHighlights(e,t)})}`
- `validate`: `({challenge:e,challengeState:t})=>this.getGuesses(e,t).every((e=>void 0!==e.guess))`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tableTokens.flat(2).map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({guess:(0,p.Xh)(e.displayTableTokens),type:e.type})`

## `tapDescribe` (TapDescribe)

- `grade`: `async({challenge:e,challengeState:t})=>{const a=this.getGuess(e,t),s=await(0,g.wM)({allowTypoBlame:!1,challenge:e,guess:a,isTypingInLearningLanguage:!1});return s.correct?s:{correct:s.correct,guess:a}}`
- `validate`: `({challenge:e,challengeState:t})=>this.getChoices(t).length===e.correctIndices.length`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.choices.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.text,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `translate` (Translate)

- `grade`: `async({challenge:e,challengeState:t,challengeToggleState:a,learningLanguage:n})=>{if(a.isToggledToTyping)return(0,g.wM)({allowTypoBlame:a.isToggledToTyping,challenge:e,guess:this.getTranslation(t),isTypingInLearningLanguage:a.isToggledToTyping&&e.grader.language===n});const r=(0,l.rF)(e),i=this.getTapTranslations(e,t),c=i.join(r),d=e.correctIndices.map((t=>e.choices[t].text));if(s.Xy(d,i))return Promise.resolve({correct:!0,guess:c});const u=await(0,g.wM)({allowTypoBlame:a.isToggledToTyping,challenge:e,guess:c,isTypingInLearningLanguage:a.isToggledToTyping&&e.grader.language===n});return u.correct?u.blameType===o.Oj.ExtraSpace?{correct:u.correct,guess:c}:u:Promise.resolve({correct:u.correct,g …`
- `validate`: `({challenge:e,challengeState:t,challengeToggleState:a})=>a.isToggledToTyping?this.getTranslation(t).length>0:this.getTapTranslations(e,t).length>0`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e,learningLanguage:t,session:a})=>s.oA([e.tts&&{autoplay:e.sourceLanguage===t,required:!1,text:e.prompt,url:e.tts},...e.choices.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.text,url:e.tts})),...e.tokens.map((e=>e.tts&&!(0,b.LJ)(a.type)&&{autoplay:!1,required:!1,text:e.value,url:e.tts}))])`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `transliterate` (Transliterate)

- `grade`: `({challenge:e,challengeState:t})=>{const a=this.getTransliteration(t),s=(e.correctSolutions||[]).map((e=>(0,d.lG)(j,e))).map((e=>e.replace(B,""))),n=(0,d.lG)(j,a).replace(B,""),r=s.includes(n);return Promise.resolve({correct:r,guess:a})}`
- `validate`: `({challengeState:e})=>this.getTransliteration(e).length>0`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `transliterationAssist` (TransliterationAssist)

- `grade`: `({challenge:e,challengeState:t})=>Promise.resolve({correct:t.guess===e.correctIndex,guess:t.guess})`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.options.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.text,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `typeCloze` (TypeCloze)

- `grade`: `async({challenge:e,challengeState:t,learningLanguage:a})=>{const n=this.getGuessedTexts(e,t),r=(0,p.Ub)(e),i=await(0,d.C3)(a);return Promise.resolve({correct:s.Xy(n.map((e=>(0,d.PW)(i.accentedCharacterMap,i.normalizationData,e))),r.map((e=>(0,d.PW)(i.accentedCharacterMap,i.normalizationData,e)))),guess:n.join(""),highlights:this.getHighlights(e,t)})}`
- `validate`: `({challenge:e,challengeState:t})=>this.getGuesses(e,t).every((e=>void 0!==e))`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tokens.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({guess:(0,p.jL)(e.displayTokens),type:e.type})`

## `typeClozeTable` (TypeClozeTable)

- `grade`: `async({challenge:e,challengeState:t,learningLanguage:a})=>{const n=this.getGuessedTexts(e,t),r=(0,p.Ub)(e),i=await(0,d.C3)(a);return{correct:s.Xy(n.map((e=>(0,d.PW)(i.accentedCharacterMap,i.normalizationData,e))),r.map((e=>(0,d.PW)(i.accentedCharacterMap,i.normalizationData,e)))),guess:n.join(""),highlights:this.getHighlights(e,t)}}`
- `validate`: `({challenge:e,challengeState:t})=>this.getGuesses(e,t).every((e=>void 0!==e.guess))`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tableTokens.flat(2).map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({guess:(0,p.Xh)(e.displayTableTokens),type:e.type})`

## `typeComplete` (TypeComplete)

- `grade`: `async({challenge:e,challengeState:t})=>{const a=e.displayTokens,n=a.findIndex((e=>e.isBlank)),r=e.displayTokens.map((e=>e.text)).join(""),i=this.getUserInput(e,t,!0),o=a.map(((t,a)=>t.isBlank?a===n?e.assistedText?e.assistedText+i[a]:i[a]:"":t.text)).join("");if(s.Xy(o,r))return Promise.resolve({correct:!0,guess:i});const l=await(0,g.wM)({allowTypoBlame:!0,challenge:e,guess:o,isTypingInLearningLanguage:!0});return l.guess=i[0],l.correct||delete l.blameMessage,l}`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tokens.map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({guess:"",type:e.type})`

## `typeCompleteTable` (TypeCompleteTable)

- `grade`: `async({challenge:e,challengeState:t,learningLanguage:a})=>{const n=this.getGuessedTexts(e,t),r=(0,p.Ub)(e),i=await(0,d.C3)(a);return{correct:s.Xy(n.map((e=>(0,d.PW)(i.accentedCharacterMap,i.normalizationData,e))),r.map((e=>(0,d.PW)(i.accentedCharacterMap,i.normalizationData,e)))),guess:n.join(""),highlights:this.getHighlights(e,t)}}`
- `validate`: `({challenge:e,challengeState:t})=>this.getGuesses(e,t).every((e=>void 0!==e.guess))`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>s.oA(e.tableTokens.flat(2).map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))`
- `prepareChallengeState`: `({challenge:e})=>({guess:(0,p.Xh)(e.displayTableTokens),type:e.type})`

## `writeComprehension` (WriteComprehension)

- `grade`: `async({challenge:e,challengeState:t,learningLanguage:a})=>{var s;const n=await(0,g.wM)({allowTypoBlame:!0,challenge:e,guess:null!==(s=t.guess)&&void 0!==s?s:"",isTypingInLearningLanguage:e.grader.language===a});return{blameMessage:n.correct?void 0:(0,m.H8)(30063),correct:n.correct,guess:n.guess}}`
- `validate`: `({challengeState:e})=>void 0!==e.guess`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `({challenge:e})=>{var t,a;return s.oA([...null!==(t=e.passageTokens)&&void 0!==t?t:[],...null!==(a=e.questionTokens)&&void 0!==a?a:[]].map((e=>e.tts&&{autoplay:!1,required:!1,text:e.value,url:e.tts})))}`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `star` (ChessStar)

- `grade`: `({challengeState:e})=>{var t;const a=e.guess,s="correct"===(null===(t=null==a?void 0:a.gradingState)||void 0===t?void 0:t.type);return Promise.resolve({correct:s,guess:a})}`
- `validate`: `()=>!1`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `normal` (ChessNormal)

- `grade`: `({challengeState:e})=>{var t;const a=e.guess,s="correct"===(null===(t=null==a?void 0:a.gradingState)||void 0===t?void 0:t.type);return Promise.resolve({correct:s,guess:a})}`
- `validate`: `()=>!1`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `ChessFictionalBoard` (ChessFictionalBoard)

- `grade`: `({challengeState:e})=>{var t;const a=e.guess,s="correct"===(null===(t=null==a?void 0:a.gradingState)||void 0===t?void 0:t.type);return Promise.resolve({correct:s,guess:a})}`
- `validate`: `()=>!1`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `chessMatch` (ChessMatch)

- `grade`: `({challengeState:e})=>{var t,a;const s=e.guess,n="completed"===(null===(t=null==s?void 0:s.matchState)||void 0===t?void 0:t.status)&&"win"===(null===(a=null==s?void 0:s.matchState)||void 0===a?void 0:a.outcome);return Promise.resolve({correct:n,guess:s})}`
- `validate`: `()=>!1`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `chessPvpMatch` (ChessPvpMatch)

- `grade`: `({challengeState:e})=>{var t,a;const s=e.guess,n="completed"===(null===(t=null==s?void 0:s.matchState)||void 0===t?void 0:t.status)&&"win"===(null===(a=null==s?void 0:s.matchState)||void 0===a?void 0:a.outcome);return Promise.resolve({correct:n,guess:s})}`
- `validate`: `()=>!1`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`

## `chessMiniMatch` (ChessMiniMatch)

- `grade`: `()=>Promise.resolve({correct:!0,guess:void 0})`
- `validate`: `()=>!1`
- `isReadyForGuessing`: `()=>!0`
- `getTts`: `()=>[]`
- `prepareChallengeState`: `({challenge:e})=>({type:e.type})`
