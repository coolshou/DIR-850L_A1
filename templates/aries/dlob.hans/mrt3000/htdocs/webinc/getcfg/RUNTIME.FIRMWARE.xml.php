<module>
	<service><?=$GETCFG_SVC?></service>
	<runtime>
		<firmware>
			<fwversion>
				<Major><? echo query("/runtime/firmware/fwversion/Major");?></Major>
				<Minor><? echo query("/runtime/firmware/fwversion/Minor");?></Minor>
			</fwversion>
		</firmware>
	</runtime>
</module>
